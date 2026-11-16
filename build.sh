#!/usr/bin/env bash
pdflatex -jobname='minibridge-rules' letter
pdflatex -jobname='minibridge-rules' letter
pdflatex -jobname='minibridge-rules-a4' a4
pdflatex -jobname='minibridge-rules-a4' a4
pdflatex -jobname='minibridge-rules-1up' '\def\oneupmode{}\input{letter}'
pdflatex -jobname='minibridge-rules-1up-a4' '\def\oneupmode{}\input{a4}'
