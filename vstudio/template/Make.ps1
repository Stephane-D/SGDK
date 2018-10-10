#
# Basic script for compiling SGDK Megadrive code.
#
# Setup to support Build, Clean and Rebuild.
#
param
(	
    [String]$Action = "",
    [String]$SourceLocation = "",
    [String]$AssemblyName = "",
    [String]$Flavour = "Release",
    [String]$SGDK = "" # Set me
)

if(-not $Action)
{
    Write-Host "No Action"
    exit -1
}

if(-not $SourceLocation)
{
    Write-Host "No Source Location"
    exit -1
}

if(-not $SGDK)
{
    Write-Host "No SGDK"
    exit -1
}


$env:GDK = $SGDK.Replace('\','/')
$env:GDK_WIN = $SGDK.Replace('/','\')
$env:Path = "$($env:Path);$($env:GDK_WIN)\bin"


Write-Host "--== SGDK Compiler Script ==--"
Write-Host "Action: $Action"
Write-Host "SourceLocation: $SourceLocation"
Write-Host "OutFolder: $OutFolder"
Write-Host "AssemblyName: $AssemblyName"
Write-Host "ObjectFolder: $ObjectFolder"
Write-Host "Flavour: $Flavour"








function BuildCode()
{
    Write-Host "Compiling Build"   

    $pinfo = New-Object System.Diagnostics.ProcessStartInfo
    $pinfo.FileName = "make.exe"
	$pinfo.RedirectStandardOutput = $false
    $pinfo.RedirectStandardError = $true    
    $pinfo.UseShellExecute = $false
    $pinfo.Arguments = "-f $($env:GDK_WIN)\makefile.gen"
    $pinfo.WorkingDirectory = $SourceLocation

    $pinfo.EnvironmentVariables["path"] = $env:Path
    $pinfo.EnvironmentVariables["GDK"] = $env:GDK
    $pinfo.EnvironmentVariables["GDK_WIN"] = $env:GDK_WIN

    $p = New-Object System.Diagnostics.Process
    

    $p.StartInfo = $pinfo
    $p.Start() | Out-Null
    
    #$stdout = $p.StandardOutput.ReadToEnd()
    $stderr = $p.StandardError.ReadToEnd()
    $p.WaitForExit()

    $LASTEXITCODE = $p.ExitCode



    Write-Host $stdout
    Write-Host $stderr

    # Parse the result for errors!

    $lines = $stderr -split "`n"

    foreach($line in $lines)
    {
        # sccz80:"E:\Programming\ZxSpectrum\Atoms\Atoms\main.c" L:43 Error:#27:Missing Open Parenthesis
        # <file_name>(row,column): error: <text> 

        if($line.Contains(" error: ") -or $line.Contains(" warning: "))
        {
            $nameParts = $line -split ":"
            
            $name = $nameParts[0]

            $LineNum = $nameParts[1]
            $charNum = $nameParts[2]

            $type = $nameParts[3].trim()
            $message = $nameParts[4]


            Write-Host "$name($LineNum,$charNum): $($type): $message"
        }
        else
        {
            if($line.Contains("Error at file"))
            {
                Write-Host $line
            }
        }
    }

    return $p.ExitCode
}


function CleanCode()
{
    Write-Host "Cleaning build"
    
    Remove-Item "out" -Recurse -Force
}


Try
{
    switch( $Action.ToLower() )
    {
        "build"
        {
            $exit = BuildCode
            Write-Host "Build Compete ($exit)"
            Exit $exit
        }

        "clean"
        {
            CleanCode
            Write-Host "Clean Compete (0)"
            Exit 0
        }


        "rebuild"
        {
            CleanCode
            $exit = BuildCode
            Write-Host "Rebuild Compete ($exit)"
            Exit $exit
        }   
    }
}
Catch
{
    Write-Host "Something went wrong: $_"
    Exit -1
}