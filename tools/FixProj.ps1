param([String]$file)

$Script_Working_path = (Get-Location).Path

$prj_path = Join-Path $Script_Working_path $file #"*.vcxproj"

$x = [xml](Get-Content $prj_path)
$nso = New-Object System.Xml.XmlNamespaceManager($x.NameTable)
$namespace = $x.DocumentElement.NamespaceURI
$ns = "ns:"
$nso.AddNamespace("ns", $namespace)

$x.SelectNodes("//ns:PlatformToolset[. = 'v120']", $nso) | foreach {
  Write-Host "PlatformToolset"
  #Write-Host ("Found toolset: " + $_.InnerText)
  $_.InnerText = "v120_xp"
  #Write-Host ("   Changed to: " + $_.InnerText)
}

$x.SelectNodes("//ns:PropertyGroup[@Condition and not(@Label)]", $nso) | foreach {
  Write-Host "OutDir, IntDir, TargetName"
  if ($_.Condition.Contains("Release")) {
    $OutDir = "`$(SolutionDir)Release\"
  } else {
    $OutDir = "`$(SolutionDir)Debug\"
  }
  $IntDir = "`$(SolutionDir)..\_build\`$(ProjectName)\`$(Platform).`$(Configuration)\"
  $TgName = "`$(ProjectName)W"

  $node = "OutDir"
  $i = $_.SelectSingleNode("ns:$node", $nso)
  if ($i -eq $null) { $i = $x.CreateElement($node, $namespace); $i = $_.AppendChild($i) }
  $i.InnerText = $OutDir

  $node = "IntDir"
  $i = $_.SelectSingleNode("ns:$node", $nso)
  if ($i -eq $null) { $i = $x.CreateElement($node, $namespace); $i = $_.AppendChild($i) }
  $i.InnerText = $IntDir

  $node = "TargetName"
  $i = $_.SelectSingleNode("ns:$node", $nso)
  if ($i -eq $null) { $i = $x.CreateElement($node, $namespace); $i = $_.AppendChild($i) }
  if (-Not ($i.InnerText.Contains("`$"))) {
    $i.InnerText = $TgName
  }
}

$x.SelectNodes("//ns:ItemDefinitionGroup[@Condition][(ns:Link or ns:ClCompile)]", $nso) | foreach {
  Write-Host "OutputFile, MinVer, SafeSeh, PDB"
  $is_unicode = $FALSE
  if ($_.Condition.Contains("Debug")) { $dbg = "_DEBUG" } else { $dbg = "NDEBUG" }

  if ($_.Condition.Contains("Win32")) {
    $MinVer = "5.01"
  } else {
    $MinVer = "5.02"
  }

  $_.SelectNodes("ns:ClCompile", $nso) | foreach {
    #Write-Host "PreprocessorDefinitions"
    $node = "PreprocessorDefinitions"
    $i = $_.SelectSingleNode("ns:$node", $nso)
    if ($i -ne $null) {
      if (($i.InnerText.Contains("UNICODE")) -Or ($i.InnerText.Contains("FARAPI18"))) {
        $is_unicode = $true
        if (-Not ($i.InnerText.Contains("FAR_UNICODE"))) {
          $i.InnerText = ("FAR_UNICODE=3000;" + $i.InnerText)
        }
      }
    }
  }

  $_.SelectNodes("ns:Link", $nso) | foreach {
    $_.SelectNodes("ns:OutputFile", $nso) | foreach {
      $_.InnerText = "`$(TargetPath)"
    }

    $_.SelectNodes("ns:ProgramDatabaseFile", $nso) | foreach {
      $_.InnerText = "`$(OutDir)`$(TargetName).pdb"
    }

    $node = "MinimumRequiredVersion"
    $i = $_.SelectSingleNode("ns:$node", $nso)
    if ($i -eq $null) { $i = $x.CreateElement($node, $namespace); $i = $_.AppendChild($i) }
    $i.InnerText = $MinVer

    if ($dbg -eq "_DEBUG") {
      $node = "ImageHasSafeExceptionHandlers"
      $i = $_.SelectSingleNode("ns:$node", $nso)
      if ($i -eq $null) { $i = $x.CreateElement($node, $namespace); $i = $_.AppendChild($i) }
      $i.InnerText = "false"
    }
  }

  if ($is_unicode) {
    $node = "ResourceCompile"
    $r = $_.SelectSingleNode("ns:$node", $nso)
    if ($r -eq $null) { $r = $x.CreateElement($node, $namespace); $r = $_.AppendChild($r) }
    $node = "PreprocessorDefinitions"
    $i = $r.SelectSingleNode("ns:$node", $nso)
    if ($i -eq $null) {
      $i = $x.CreateElement($node, $namespace); $i = $r.AppendChild($i)
      $i.InnerText = "FAR_UNICODE=3000;_UNICODE;%(PreprocessorDefinitions)"
    } else {
      if (-Not ($i.InnerText.Contains("_UNICODE"))) { $i.InnerText = ("_UNICODE;" + $i.InnerText) }
      $i.InnerText = ("FAR_UNICODE=3000;" + $i.InnerText)
    }
  }
}

Write-Host "Saving project"
$x.Save($prj_path)
