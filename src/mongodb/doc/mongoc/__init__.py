from docutils.nodes import literal, Text
from docutils.parsers.rst import roles

from sphinx.roles import XRefRole
from sphinx import version_info as sphinx_version_info


class SymbolRole(XRefRole):
    def __call__(self, *args, **kwargs):
        nodes, messages = XRefRole.__call__(self, *args, **kwargs)
        for node in nodes:
            attrs = node.attributes
            target = attrs['reftarget']
            parens = ''
            if target.endswith('()'):
                # Function call, :symbol:`mongoc_init()`
                target = target[:-2]
                parens = '()'

            if ':' in target:
                # E.g., 'bson:bson_t' has domain 'bson', target 'bson_t'
                attrs['domain'], name = target.split(':', 1)
                attrs['reftarget'] = name

                old = node.children[0].children[0]
                assert isinstance(old, Text)
                new = Text(name + parens, name + parens)
                # Ensure setup_child is called.
                node.children[0].replace(old, new)

            else:
                attrs['reftarget'] = target

            attrs['reftype'] = 'doc'
            attrs['classes'].append('symbol')

            if sphinx_version_info >= (1, 6):
                # https://github.com/sphinx-doc/sphinx/issues/3698
                attrs['refdomain'] = 'std'

        return nodes, messages


def setup(app):
    roles.register_local_role(
        'symbol', SymbolRole(warn_dangling=True, innernodeclass=literal))

    return {
        'version': '1.0',
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
