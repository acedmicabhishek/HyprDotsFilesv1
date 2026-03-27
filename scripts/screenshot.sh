#!/bin/bash
# Capture selection and pipe to Swappy for editing
grim -g "$(slurp)" - | swappy -f -
