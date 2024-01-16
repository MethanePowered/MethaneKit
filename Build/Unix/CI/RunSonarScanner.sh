#!/bin/bash
# CI script to run Sonar Scanner code analysis
sonar_project_key="${1}"
build_dir="${2}"
tests_dir="${3}"
if [ "${SONAR_TOKEN}" == "" ]; then
    echo "Sonar Token is not available!"
    exit 1
fi
case "$OSTYPE" in
  msys*|cygwin*) sonar_scanner_exe="sonar-scanner.bat" ;;
  *) sonar_scanner_exe="sonar-scanner" ;;
esac
# Add -X flag to enable debug output:
SONAR_SCAN_CMD="${sonar_scanner_exe} --define sonar.host.url=https://sonarcloud.io"
SONAR_SCAN_CMD="$SONAR_SCAN_CMD --define sonar.organization=${SONAR_ORGANIZATION}"
SONAR_SCAN_CMD="$SONAR_SCAN_CMD --define sonar.projectKey=${sonar_project_key}"
SONAR_SCAN_CMD="$SONAR_SCAN_CMD --define sonar.projectVersion=${METHANE_VERSION_MAJOR}.${METHANE_VERSION_MINOR}.${METHANE_VERSION_PATCH}.${METHANE_VERSION_BUILD}"
SONAR_SCAN_CMD="$SONAR_SCAN_CMD --define sonar.cfamily.compile-commands=${build_dir}/compile_commands.json"
SONAR_SCAN_CMD="$SONAR_SCAN_CMD --define sonar.testExecutionReportPaths=${test_results}"
SONAR_SCAN_CMD="$SONAR_SCAN_CMD --define sonar.coverageReportPaths=${tests_dir}/Coverage/Report/SonarQube.xml"
SONAR_SCAN_CMD="$SONAR_SCAN_CMD --define sonar.scm.revision=${GITHUB_COMMIT_SHA}"
if [ ${GITHUB_PR_FLAG} -gt 0 ]; then
  SONAR_SCAN_CMD="$SONAR_SCAN_CMD --define sonar.pullrequest.provider=GitHub"
  SONAR_SCAN_CMD="$SONAR_SCAN_CMD --define sonar.pullrequest.github.repository=${GITHUB_PR_REPO}"
  SONAR_SCAN_CMD="$SONAR_SCAN_CMD --define sonar.pullrequest.key=${GITHUB_PR_NUMBER}"
  SONAR_SCAN_CMD="$SONAR_SCAN_CMD --define sonar.pullrequest.branch=${GITHUB_PR_BRANCH}"
  SONAR_SCAN_CMD="$SONAR_SCAN_CMD --define sonar.pullrequest.base=${GITHUB_PR_BASE}"
fi
set -o pipefail
echo "Git-Hub pull-request flag: '${GITHUB_PR_FLAG}'" | tee $SCAN_LOG_FILE
echo "$SONAR_SCAN_CMD" | tee $SCAN_LOG_FILE
eval "$SONAR_SCAN_CMD" 2>&1 | tee -a $SCAN_LOG_FILE
cp sonar-project.properties $INSTALL_DIR
prop_files=(`find .sonar -name "sonar-scanner.properties"`)
for prop_file in "${prop_files[@]}"; do
  cp $prop_file $INSTALL_DIR
done
