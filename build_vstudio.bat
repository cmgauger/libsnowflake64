@echo off
MSBuild /nologo libsnowflake64.sln /Target:Build /Property:Configuration=Debug /Property:Platform=Win32
MSBuild /nologo libsnowflake64.sln /Target:Build /Property:Configuration=Debug /Property:Platform=x64
MSBuild /nologo libsnowflake64.sln /Target:Build /Property:Configuration=Release /Property:Platform=Win32
MSBuild /nologo libsnowflake64.sln /Target:Build /Property:Configuration=Release /Property:Platform=x64
