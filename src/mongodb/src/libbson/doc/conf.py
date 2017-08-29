# -*- coding: utf-8 -*-
import os.path
import sys

from sphinx import version_info as sphinx_version_info

# Ensure we can import "mongoc" and "taglist" extension modules.
sys.path.append(os.path.dirname(__file__))

extensions = [
    'mongoc',
    'taglist',
    'sphinx.ext.extlinks',
]

# General information about the project.
project = 'Libbson'
copyright = '2017, MongoDB, Inc'
author = 'MongoDB, Inc'
googleanalytics_id = 'UA-92642455-1'

version_path = os.path.join(os.path.dirname(__file__), '..', 'VERSION_CURRENT')
version = open(version_path).read().strip()
release_path = os.path.join(os.path.dirname(__file__), '..', 'VERSION_RELEASED')
release = open(release_path).read().strip()
release_major, release_minor, release_patch = release.split('.')
release_download = 'https://github.com/mongodb/libbson/releases/download/{0}/libbson-{0}.tar.gz'.format(release)
rst_prolog = """
.. |release_major| replace:: %(release_major)s

.. |release_minor| replace:: %(release_minor)s

.. |release_patch| replace:: %(release_patch)s

.. |release_download| replace:: https://github.com/mongodb/libbson/releases/download/%(release)s/libbson-%(release)s.tar.gz
""" % locals()

# The extension requires the "base" to contain '%s' exactly once, but we never intend to use it though
extlinks = {'release': (release_download+'%s', '')}

language = 'en'
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']
master_doc = 'index'

# -- Options for HTML output ----------------------------------------------

html_theme_path = ['.']
html_theme = 'mongoc-theme'
html_title = html_shorttitle = 'libbson %s' % version
# html_favicon = None

if sphinx_version_info >= (1, 6):
    smart_quotes = False
else:
    html_use_smartypants = False

html_sidebars = {
    '**': ['globaltoc.html'],
    'errors': [],  # Make more room for the big table.
}
html_show_sourcelink = False

# Note: http://www.sphinx-doc.org/en/1.5.1/config.html#confval-html_copy_source
# This will degrade the Javascript quicksearch if we ever use it.
html_copy_source = False

# -- Options for manual page output ---------------------------------------

# HACK: Just trick Sphinx's ManualPageBuilder into thinking there are pages
# configured - we'll do it dynamically in process_nodes.
man_pages = [True]

# If true, show URL addresses after external links.
#
# man_show_urls = False
from docutils.nodes import title


# To publish HTML docs at GitHub Pages, create .nojekyll file. In Sphinx 1.4 we
# could use the githubpages extension, but old Ubuntu still has Sphinx 1.3.
def create_nojekyll(app, env):
    if app.builder.format == 'html':
        path = os.path.join(app.builder.outdir, '.nojekyll')
        with open(path, 'wt') as f:
            f.write('foo')


def add_ga_javascript(app, pagename, templatename, context, doctree):
    context['metatags'] = context.get('metatags', '') + """<script>
  (function(i,s,o,g,r,a,m){i['GoogleAnalyticsObject']=r;i[r]=i[r]||function(){
  (i[r].q=i[r].q||[]).push(arguments)},i[r].l=1*new Date();a=s.createElement(o),
  m=s.getElementsByTagName(o)[0];a.async=1;a.src=g;m.parentNode.insertBefore(a,m)
  })(window,document,'script','https://www.google-analytics.com/analytics.js','ga');

  ga('create', '%s', 'auto');
  ga('send', 'pageview');

</script>""" % googleanalytics_id


def add_canonical_link(app, pagename, templatename, context, doctree):
    link = ('<link rel="canonical"'
            ' href="http://mongoc.org/libbson/current/%s.html"/>' % pagename)

    context['metatags'] = context.get('metatags', '') + link


def setup(app):
    app.connect('doctree-read', process_nodes)
    app.connect('env-updated', create_nojekyll)
    app.connect('html-page-context', add_ga_javascript)
    app.connect('html-page-context', add_canonical_link)


def process_nodes(app, doctree):
    if man_pages == [True]:
        man_pages.pop()

    env = app.env
    metadata = env.metadata[env.docname]

    # A page like installing.rst sets its name with ":man_page: bson_installing"
    page_name = metadata.get('man_page')
    if not page_name:
        print('Not creating man page for %s' % env.docname)
        return

    page_title = find_node(doctree, title)

    man_pages.append((env.docname, page_name, page_title.astext(), [author], 3))


def find_node(doctree, klass):
    matches = doctree.traverse(lambda node: isinstance(node, klass))
    if not matches:
        raise IndexError("No %s in %s" % (klass, doctree))

    return matches[0]
