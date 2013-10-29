#!/usr/bin/env python
"""
dfu setup

Install script for the Ubertooth spectrum analyzer tool

Usage: python setup.py install

This file is part of project Ubertooth
Copyright 2012 Dominic Spill
"""

from distutils.core import setup

setup(
    name        = "specan",
    description = "A tool for reading spectrum analyzer data from an Ubertooth device",
    author      = "Jared Boone, Michael Ossmann, Dominic Spill",
    url         = "https://sourceforge.net/projects/ubertooth/",
    license     = "GPL",
    version     = '',
    packages    = ['specan'],
    scripts     = ['ubertooth-specan-ui'],
    classifiers=[
        'Development Status :: 5 - Beta',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: GNU General Public License (GPL)',
        'Programming Language :: Python',
        'Operating System :: OS Independent',
    ],
)
