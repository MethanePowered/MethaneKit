$SONAR_SCANNER_VERSION = $args[0]
$SONAR_DIRECTORY = $args[1]
$SONAR_SCANNER_HOME = "$SONAR_DIRECTORY/sonar-scanner-$SONAR_SCANNER_VERSION-windows"
rm $SONAR_SCANNER_HOME -Force -Recurse -ErrorAction SilentlyContinue
New-Item -path $SONAR_SCANNER_HOME -type directory
(New-Object System.Net.WebClient).DownloadFile("https://binaries.sonarsource.com/Distribution/sonar-scanner-cli/sonar-scanner-cli-$SONAR_SCANNER_VERSION-windows.zip", "$SONAR_DIRECTORY/sonar-scanner.zip")
Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::ExtractToDirectory("$SONAR_DIRECTORY/sonar-scanner.zip", "$SONAR_DIRECTORY")
rm ./.sonar/sonar-scanner.zip -Force -ErrorAction SilentlyContinue

rm "$SONAR_DIRECTORY/build-wrapper-win-x86" -Force -Recurse -ErrorAction SilentlyContinue
(New-Object System.Net.WebClient).DownloadFile("https://sonarcloud.io/static/cpp/build-wrapper-win-x86.zip", "$SONAR_DIRECTORY/build-wrapper-win-x86.zip")
[System.IO.Compression.ZipFile]::ExtractToDirectory("$SONAR_DIRECTORY/build-wrapper-win-x86.zip", "$SONAR_DIRECTORY")