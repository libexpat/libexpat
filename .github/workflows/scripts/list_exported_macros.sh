#! /usr/bin/env bash
grep -Eo '(define|undef) [a-zA-Z0-9_]+' "${1}" | awk '{print $2}' | sort -u
