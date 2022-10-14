# Configuration file for the Sphinx documentation builder.
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# Project information

project = 'Monocle'
copyright = '2022, BrilliantLabs Inc.'
author = 'BrilliantLabs Inc.'
release = 'v0.0'

# General configuration

extensions = [
  "sphinxcontrib.doxylink",
]

templates_path = ['_templates']
exclude_patterns = []

# HTML output

html_theme = 'alabaster'
html_static_path = ['_static']

# DoxySphinx

doxylink = {
    "driver": (
        "source/driver/html/tagfile.xml",
        "source/driver/html",
    ),
}
