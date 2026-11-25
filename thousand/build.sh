#!/usr/bin/env bash
# Stop at the first LaTeX error instead of waiting for keyboard input.
set -e
pdflatex -interaction=nonstopmode -halt-on-error -jobname='thousand-rules' letter
pdflatex -interaction=nonstopmode -halt-on-error -jobname='thousand-rules' letter
pdflatex -interaction=nonstopmode -halt-on-error -jobname='thousand-rules-a4' a4
pdflatex -interaction=nonstopmode -halt-on-error -jobname='thousand-rules-a4' a4
pdflatex -interaction=nonstopmode -halt-on-error -jobname='thousand-rules-1up' '\def\oneupmode{}\input{letter}'
pdflatex -interaction=nonstopmode -halt-on-error -jobname='thousand-rules-1up-a4' '\def\oneupmode{}\input{a4}'
