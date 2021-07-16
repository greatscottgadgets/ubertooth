# -- Project information -----------------------------------------------------

project = 'Ubertooth'
copyright = '2021, Great Scott Gadgets'
author = 'Great Scott Gadgets'

# -- General configuration ---------------------------------------------------

extensions = [
    'sphinx.ext.autodoc'
]

templates_path = ['_templates']
source_suffix = '.rst'
master_doc = 'index'
language = None
exclude_patterns = ['_build']
pygments_style = None


# -- Options for HTML output -------------------------------------------------

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']

