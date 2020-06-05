from setuptools import setup

with open("README.md", "r") as fh:
    long_description = fh.read()

setup(
	name="ubtbr",
	version="1.0",
	packages=['ubtbr'],
	description="Ubtbr firmware control library",
	long_description=long_description,
	long_description_content_type="text/markdown",
	install_requires=['libusb1'],
	scripts=["ubertooth-btbr"],
	url="https://github.com/greatscottgadgets/ubertooth/",
	author="Etienne Helluy-Lafont, Univ. Lille, CNRS",
	python_requires='>=3',
	classifiers=[
		'Development Status :: 5 - Beta',
		'Intended Audience :: Developers',
		'License :: OSI Approved :: GNU General Public License (GPL)',
		'Programming Language :: Python',
		'Operating System :: OS Independent',
	],
)
