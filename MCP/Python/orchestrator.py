import logging
import json
import os
import sys
from typing import List, Optional, Dict, Any

from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
from openai import OpenAI

# Add current directory to path to import unreal_mcp_server safely
current_dir = os.path.dirname(os.path.abspath(__file__))
if current_dir not in sys.path:
    sys.path.append(current_dir)

try:
    from unreal_mcp_server import get_unreal_connection, mcp
except ImportError:
    print("Error: Could not import unreal_mcp_server. Make sure orchestrator.py is in the same directory as unreal_mcp_server.py")
    sys.exit(1)

import traceback

# Configuration
# Please ensure you have pulled this model in Ollama: `ollama pull qwen3:8b`
MODEL_NAME = "qwen3:4b" 
OLLAMA_API_URL = "http://localhost:11434/v1"
ORCHESTRATOR_PORT = 8000

SYSTEM_PROMPT = """You are an expert Unreal Engine 5 assistant. 
You can access the Unreal Editor via the provided tools. 
Answer user questions and perform tasks by calling the appropriate tools. 
If a task requires multiple steps, perform them one by one.
Always try to be helpful and precise."""

# Logging setup
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger("Orchestrator")

app = FastAPI(title="Unreal AI Orchestrator")

from fastapi import Request
from fastapi.responses import JSONResponse

@app.exception_handler(Exception)
async def global_exception_handler(request: Request, exc: Exception):
    error_msg = f"Global Exception: {str(exc)}\n{traceback.format_exc()}"
    logger.error(error_msg)
    return JSONResponse(
        status_code=500,
        content={"detail": str(exc), "traceback": traceback.format_exc()},
    )

# OpenAI Client for Ollama
client = OpenAI(
    base_url=OLLAMA_API_URL,
    api_key="ollama" # Required but ignored by Ollama
)

# Request Models
class Message(BaseModel):
    role: str
    content: str
    tool_calls: Optional[List[Any]] = None

class ChatRequest(BaseModel):
    messages: List[Message]
    model: Optional[str] = MODEL_NAME

# Tool Definitions (Mapping to unreal_mcp_server tools)
import asyncio
import inspect

def load_mcp_tools():
    """Dynamically load tools from the imported MCP server instance."""
    try:
        print("Loading tools from MCP server...")
        # Check if list_tools is async
        if inspect.iscoroutinefunction(mcp.list_tools):
            # We are at module level, so we can run a temporary loop
            raw_tools = asyncio.run(mcp.list_tools())
        else:
            raw_tools = mcp.list_tools()
            
        openai_tools = []
        for tool in raw_tools:
            # Convert FastMCP tool definition to OpenAI format
            # FastMCP tools are Pydantic models with: name, description, inputSchema
            if hasattr(tool, "model_dump"):
                tool_data = tool.model_dump()
            else:
                tool_data = tool
                
            # Map fields
            # FastMCP: inputSchema -> OpenAI: parameters
            function_def = {
                "name": tool_data.get("name"),
                "description": tool_data.get("description") or "",
                "parameters": tool_data.get("inputSchema", {})
            }
            
            openai_tools.append({
                "type": "function",
                "function": function_def
            })
            
        print(f"Successfully loaded {len(openai_tools)} tools from MCP server.")
        return openai_tools
        
    except Exception as e:
        logger.error(f"Failed to load tools from MCP server: {e}")
        traceback.print_exc()
        return []

# Load tools dynamically
TOOLS = load_mcp_tools()

@app.post("/chat")
def chat(request: ChatRequest):
    """
    Chat endpoint receiving messages from Unreal Engine (or any client).
    It forwards logic to Ollama and executes tools via Unreal MCP connection.
    Supports multi-turn tool calls.
    """
    
    # 1. Construct Messages for OpenAI API
    # Ensure system prompt exists
    messages = [msg.model_dump(exclude_none=True) for msg in request.messages]
    
    # Simple check to add system prompt if not present at the start
    if not messages or messages[0].get("role") != "system":
        messages.insert(0, {
            "role": "system", 
            "content": SYSTEM_PROMPT
        })

    max_turns = 10  # Safety limit for tool call loops
    turn_count = 0

    try:
        while turn_count < max_turns:
            turn_count += 1
            print(f"[DEBUG] Turn {turn_count}/{max_turns} with {len(messages)} messages")
            logger.info(f"Turn {turn_count}/{max_turns} with {len(messages)} messages")

            # 2. Inference: Ask LLM what to do
            response = client.chat.completions.create(
                model=request.model,
                messages=messages,
                tools=TOOLS,
                tool_choice="auto"
            )
            
            assistant_msg = response.choices[0].message
            
            # Sanitize logging
            try:
                safe_msg = str(assistant_msg).encode('ascii', errors='backslashreplace').decode('ascii')
                print(f"[DEBUG] Model Output: {safe_msg}")
                if assistant_msg.content:
                    print(f"[DEBUG] Model Thoughts: {assistant_msg.content}")
            except Exception:
                safe_msg = "(Encoding Error)"

            # 3. Check for Tool Calls
            if assistant_msg.tool_calls:
                logger.info(f"LLM triggered {len(assistant_msg.tool_calls)} tool calls")
                
                # Append the assistant's request (with tool_calls) to history
                messages.append(assistant_msg)
                
                # Connect to Unreal Engine
                unreal = get_unreal_connection()
                if not unreal:
                    err_msg = "Error: Could not connect to Unreal Engine."
                    logger.error(err_msg)
                    return {"role": "assistant", "content": err_msg}

                # Execute each tool
                for tool_call in assistant_msg.tool_calls:
                    func_name = tool_call.function.name
                    try:
                        func_args = json.loads(tool_call.function.arguments)
                    except json.JSONDecodeError:
                        func_args = {}
                    
                    print(f"[DEBUG] Executing Tool: {func_name} | Args: {func_args}")
                    logger.info(f"Executing Tool: {func_name}")
                    
                    # EXECUTE VIA UNREAL CONNECTION
                    ue_response = unreal.send_command(func_name, func_args)
                    
                    # Format result
                    result_content = json.dumps(ue_response) if ue_response else "No response"
                    print(f"[DEBUG] Tool Result: {result_content}")
                    logger.info(f"Tool Result: {result_content}")
                    
                    # Append result to messages
                    messages.append({
                        "tool_call_id": tool_call.id,
                        "role": "tool",
                        "name": func_name,
                        "content": result_content
                    })

                # Loop continues to next turn to let LLM see results and decide next step
                continue
            
            else:
                # 4. No tool calls -> Final Response
                final_content = assistant_msg.content
                logger.info("No more tool calls. Validating final response.")

                # Sanitize final response
                try:
                    safe_final = str(final_content).encode('ascii', errors='backslashreplace').decode('ascii')
                    print(f"[DEBUG] Final Response: {safe_final}")
                except:
                    pass

                return {
                    "role": "assistant",
                    "content": final_content
                }
        
        # If loop finishes without returning (max turns reached)
        logger.warning("Max turns reached without final response.")
        return {
            "role": "assistant", 
            "content": "I apologize, but the task required too many steps and I had to stop."
        }

    except Exception as e:
        logger.error(f"Error during processing: {e}")
        logger.error(traceback.format_exc())
        raise HTTPException(status_code=500, detail=str(e))

if __name__ == "__main__":
    import uvicorn
    print(f"Starting Orchestrator Server on http://0.0.0.0:{ORCHESTRATOR_PORT}")
    print(f"Targeting Ollama URL: {OLLAMA_API_URL}")
    print(f"Targeting Model: {MODEL_NAME}")
    uvicorn.run(app, host="0.0.0.0", port=ORCHESTRATOR_PORT)
