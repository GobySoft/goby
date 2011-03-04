#!/bin/bash

set -e

if [[ "$1" == "quick" ]]; then
    xelatex -halt-on-error user_manual.tex
else
    xelatex user_manual.tex -halt-on-error -no-pdf
    bibtex user_manual
    makeglossaries user_manual
    xelatex user_manual.tex -halt-on-error -no-pdf
    xelatex user_manual.tex
    gnome-open user_manual.pdf
fi