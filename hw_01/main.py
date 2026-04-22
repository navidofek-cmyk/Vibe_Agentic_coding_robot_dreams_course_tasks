import json

from openai import OpenAI

from api import API_KEY


client = OpenAI(api_key=API_KEY)


def calculate(expression: str) -> str:
    allowed_chars = "0123456789+-*/(). "
    if not all(char in allowed_chars for char in expression):
        return "Error: expression contains unsupported characters."

    try:
        result = eval(expression, {"__builtins__": {}}, {})
        return str(result)
    except Exception as error:
        return f"Error while calculating: {error}"


TOOLS = [
    {
        "type": "function",
        "function": {
            "name": "calculate",
            "description": "Evaluate a basic arithmetic expression.",
            "parameters": {
                "type": "object",
                "properties": {
                    "expression": {
                        "type": "string",
                        "description": "Arithmetic expression, e.g. '(12 + 8) / 5'",
                    }
                },
                "required": ["expression"],
            }
        }
    }
]


def main() -> None:
    user_prompt = "Kolik je (25 + 17) * 3? Spocti to pomoci nastroje a odpovez cesky."

    messages = [
        {
            "role": "system",
            "content": "Jsi uzitecny AI asistent. Kdyz je potreba vypocet, pouzij nastroj calculate.",
        },
        {
            "role": "user",
            "content": user_prompt,
        }
    ]

    first_response = client.chat.completions.create(
        model="gpt-4.1-mini",
        messages=messages,
        tools=TOOLS,
        tool_choice="auto",
    )

    assistant_message = first_response.choices[0].message
    messages.append(assistant_message)

    if not assistant_message.tool_calls:
        print("Model did not call any tool.")
        print(assistant_message.content)
        return

    for tool_call in assistant_message.tool_calls:
        function_name = tool_call.function.name
        arguments = json.loads(tool_call.function.arguments)

        if function_name == "calculate":
            tool_result = calculate(arguments["expression"])
        else:
            tool_result = "Unknown tool."

        messages.append(
            {
                "role": "tool",
                "tool_call_id": tool_call.id,
                "content": tool_result,
            }
        )

    second_response = client.chat.completions.create(
        model="gpt-4.1-mini",
        messages=messages,
    )

    print("Final answer:")
    print(second_response.choices[0].message.content)


if __name__ == "__main__":
    main()
