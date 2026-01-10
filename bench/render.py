"""Jinja2 template rendering utilities."""

from __future__ import annotations

from pathlib import Path
from typing import Any

from jinja2 import Environment, FileSystemLoader, select_autoescape


def get_template_env() -> Environment:
    """Get the Jinja2 environment for rendering templates."""
    template_dir = Path(__file__).parent / "templates"
    return Environment(
        loader=FileSystemLoader(template_dir),
        autoescape=select_autoescape(["html", "xml"]),
    )


def render_template(template_name: str, **context: Any) -> str:
    """Render a template with the given context."""
    env = get_template_env()
    template = env.get_template(template_name)
    return template.render(**context)
