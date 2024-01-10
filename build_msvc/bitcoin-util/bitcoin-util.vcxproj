<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <Import Project="..\common.init.vcxproj" />
  <PropertyGroup Label="Globals">
    <ProjectGuid>{57A04EC9-542A-4E40-83D0-AC3BE1F36805}</ProjectGuid>
  </PropertyGroup>
  <PropertyGroup Label="Configuration">
    <ConfigurationType>Application</ConfigurationType>
    <OutDir>$(SolutionDir)$(Platform)\$(Configuration)\</OutDir>
  </PropertyGroup>
  <ItemGroup>
    <ClCompile Include="..\..\src\bitcoin-util.cpp" />
  </ItemGroup>
  <ItemGroup>
    <ProjectReference Include="..\libbitcoin_consensus\libbitcoin_consensus.vcxproj">
      <Project>{2b384fa8-9ee1-4544-93cb-0d733c25e8ce}</Project>
    </ProjectReference>
    <ProjectReference Include="..\libbitcoin_common\libbitcoin_common.vcxproj">
      <Project>{7c87e378-df58-482e-aa2f-1bc129bc19ce}</Project>
    </ProjectReference>
    <ProjectReference Include="..\libbitcoin_crypto\libbitcoin_crypto.vcxproj">
      <Project>{6190199c-6cf4-4dad-bfbd-93fa72a760c1}</Project>
    </ProjectReference>
    <ProjectReference Include="..\libbitcoin_util\libbitcoin_util.vcxproj">
      <Project>{b53a5535-ee9d-4c6f-9a26-f79ee3bc3754}</Project>
    </ProjectReference>
    <ProjectReference Include="..\libunivalue\libunivalue.vcxproj">
      <Project>{5724ba7d-a09a-4ba8-800b-c4c1561b3d69}</Project>
    </ProjectReference>
    <ProjectReference Include="..\libsecp256k1\libsecp256k1.vcxproj">
      <Project>{bb493552-3b8c-4a8c-bf69-a6e7a51d2ea6}</Project>
    </ProjectReference>
  </ItemGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
  <Import Project="..\common.vcxproj" />
</Project>
