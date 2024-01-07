#!/bin/bash
# CI script to update GitHub badge parameters depending on build status
# Badge action: https://github.com/marketplace/actions/dynamic-badges
job_status="${1}"
case "${job_status}" in
  "success")
    echo "badge_message=passed" >> $GITHUB_ENV
    echo "badge_color=#56a93c"  >> $GITHUB_ENV
    ;;
  "failure")
    echo "badge_message=failed" >> $GITHUB_ENV
    echo "badge_color=#cd6e57"  >> $GITHUB_ENV
    ;;
  "cancelled")
    echo "badge_message=cancelled" >> $GITHUB_ENV
    echo "badge_color=#9b9b9c"     >> $GITHUB_ENV
    ;;
  *)
    echo "badge_message=undefined" >> $GITHUB_ENV
    echo "badge_color=purple"      >> $GITHUB_ENV
    ;;
esac
