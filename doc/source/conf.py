# Configuration file for the Sphinx documentation builder.
# https://www.sphinx-doc.org/en/master/usage/configuration.html

project = "Monocle"
copyright = "2022, BrilliantLabs Inc."
author = "BrilliantLabs Inc."
release = "v0.0"

extensions = [
    "sphinx.ext.duration",
    "sphinx.ext.doctest",
    "sphinx.ext.autodoc",
    "sphinx.ext.autosummary",
    "sphinx.ext.intersphinx",
    "sphinxcontrib.doxylink",
]

templates_path = ["_templates"]
exclude_patterns = []

html_theme = "sphinx_rtd_theme"
html_static_path = ["_static"]

doxylink = {
    "drivers": (
        "source/drivers/html/tagfile.xml",
        "source/drivers/html",
    ),
}
