#!/bin/bash

# Adding a space after the colon makes it easier to read when typing
read -p "Enter commit comment: " comment 

# Exit the script if the user just presses Enter without typing a message
if [ -z "$comment" ]; then
  echo "Error: Commit message cannot be empty."
  exit 1
fi

git add .
git commit -m "$comment"
git push origin main
