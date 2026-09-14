param(
    [string]$TestBinary,
    [string]$LogFile,
    [string]$JunitFile
)

$log = & $TestBinary 2>&1
$status = $LASTEXITCODE
$log | Tee-Object -FilePath $LogFile

$cases = foreach ($entry in $log) {
    if ($entry -match '^(.*?):([0-9]+):([^:]+):(PASS|FAIL|IGNORE)(?::\s*(.*))?$') {
        [pscustomobject]@{
            File = $matches[1]
            Line = $matches[2]
            Name = $matches[3]
            Status = $matches[4]
            Message = $matches[5]
        }
    }
}

$xml = New-Object System.Xml.XmlDocument
$declaration = $xml.CreateXmlDeclaration('1.0', 'UTF-8', $null)
[void]$xml.AppendChild($declaration)
$suites = $xml.CreateElement('testsuites')
[void]$suites.SetAttribute('tests', [string]$cases.Count)
[void]$suites.SetAttribute('failures', [string](@($cases | Where-Object Status -eq 'FAIL').Count))
[void]$suites.SetAttribute('skipped', [string](@($cases | Where-Object Status -eq 'IGNORE').Count))
$suite = $xml.CreateElement('testsuite')
[void]$suite.SetAttribute('name', 'unit_tests')
[void]$suite.SetAttribute('tests', $suites.GetAttribute('tests'))
[void]$suite.SetAttribute('failures', $suites.GetAttribute('failures'))
[void]$suite.SetAttribute('skipped', $suites.GetAttribute('skipped'))

foreach ($case in $cases) {
    $test = $xml.CreateElement('testcase')
    [void]$test.SetAttribute('classname', [System.IO.Path]::GetFileNameWithoutExtension($case.File))
    [void]$test.SetAttribute('name', $case.Name)
    [void]$test.SetAttribute('time', '0.000')
    if ($case.Status -eq 'FAIL' -or $case.Status -eq 'IGNORE') {
        $result = $xml.CreateElement($case.Status.ToLowerInvariant())
        [void]$result.SetAttribute('message', $case.Message)
        [void]$result.AppendChild($xml.CreateTextNode($case.File + ':' + $case.Line + ': ' + $case.Message))
        [void]$test.AppendChild($result)
    }
    [void]$suite.AppendChild($test)
}

[void]$suites.AppendChild($suite)
[void]$xml.AppendChild($suites)
$xml.Save($JunitFile)
exit $status