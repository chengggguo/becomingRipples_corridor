#!/bin/bash

cd "$(dirname "$0")" || exit 1
clear
printf '%s\n' 'TF-Luna Configuration Wizard'

if ! command -v python3 >/dev/null 2>&1; then
    printf '\n%s\n' 'Python 3 was not found.'
    printf '%s\n' 'Install Python 3 from https://www.python.org/downloads/macos/'
    printf '\nPress Return to close.'
    read -r _answer
    exit 1
fi

if ! python3 -c 'import sys; raise SystemExit(0 if sys.version_info >= (3,8) else 1)'; then
    printf '\n%s\n' 'Python 3.8 or newer is required.'
    printf '%s\n' 'Install a current Python 3 from https://www.python.org/downloads/macos/'
    printf '\nPress Return to close.'
    read -r _answer
    exit 1
fi

python3 "$(dirname "$0")/configure_tf_luna.py" --wizard
wizard_status=$?

printf '\nPress Return to close.'
read -r _answer
exit "$wizard_status"
