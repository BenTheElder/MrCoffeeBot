#!/usr/bin/env python
"""
Copyright 2016 Benjamin Elder (BenTheElder)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""
from __future__ import print_function
from subprocess import call
import sys
import os

def call_and_echo(*args):
    print("Calling: ", *args)
    call(*args)

def main():
    call_and_echo(["mbed", "compile", "-t", "GCC_ARM", "-m", "LPC1768"])

if __name__ == "__main__":
    main()