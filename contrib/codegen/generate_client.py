#!/usr/bin/env python3

import argparse
import json
import os
import subprocess
from jinja2 import Environment, FileSystemLoader

def get_schema():
    """Get the schema from the Bitcoin Core RPC help."""
    try:
        result = subprocess.run(['bitcoin-cli', 'schema'], capture_output=True, text=True, check=True)
        schema = json.loads(result.stdout)
        return schema
    except Exception as e:
        raise RuntimeError("Failed to get schema from bitcoin-cli: " + str(e))
    
def sanitize_argument_names(rpcs):
    for rpc_name, details in rpcs.items():
        if "argument_names" in details:
            sanitized = []
            for arg in details["argument_names"]:
                sanitized.append(arg.split("|")[0])
            details["argument_names"] = sanitized
    
    return rpcs
    
def generate_client(schema, language, output_dir):
    """Generate client code using the specified language template."""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    templates_path = os.path.join(script_dir, "templates")
    env = Environment(loader=FileSystemLoader(templates_path))

    # choose template based on target language
    if language.lower() == "python":
        template_file = env.get_template("python_client.jinja2")
        output_file_name = "bitcoin_rpc_client.py"
    elif language.lower() == "cpp":
        template_file = env.get_template("cpp_client.jinja2")
        output_file_name = "bitcoin_rpc_client.cpp"
    else:
        raise ValueError("Unsupported language: " + language)
    
    template = env.get_template(template_file)
    rendered_code = template.render(rpcs=schema.get("rpcs", {}))

    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, output_file_name)
    with open(output_path, "w") as f:
        f.write(rendered_code)
    print(f"Generated {language.upper()} client code at {output_path}")

def main():
    parser = argparse.ArgumentParser(description="Generate Bitcoin Core RPC client code.")
    parser.add_argument('--language', required=True, choices=["python", "cpp"], help="The target language for the client code.")
    parser.add_argument('--output-dir', default="generated", help="The directory to write the generated code to.")
    args = parser.parse_args()

    print("Getting schema from Bitcoin Core RPC...")
    schema = get_schema()
    rpcs = schema.get("rpcs", {})
    rpcs = sanitize_argument_names(rpcs)
    generate_client(schema, args.language, args.output_dir)

if __name__ == "__main__":
    main()