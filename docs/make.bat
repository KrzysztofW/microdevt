@ECHO OFF
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinespushd %~dp0
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesREM Command file for Sphinx documentation
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesif "%SPHINXBUILD%" == "" (
               # Replace Windows newlines with Unix newlines	set SPHINXBUILD=sphinx-build
               # Replace Windows newlines with Unix newlines)
               # Replace Windows newlines with Unix newlinesset SOURCEDIR=.
               # Replace Windows newlines with Unix newlinesset BUILDDIR=_build
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlinesif "%1" == "" goto help
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines%SPHINXBUILD% >NUL 2>NUL
               # Replace Windows newlines with Unix newlinesif errorlevel 9009 (
               # Replace Windows newlines with Unix newlines	echo.
               # Replace Windows newlines with Unix newlines	echo.The 'sphinx-build' command was not found. Make sure you have Sphinx
               # Replace Windows newlines with Unix newlines	echo.installed, then set the SPHINXBUILD environment variable to point
               # Replace Windows newlines with Unix newlines	echo.to the full path of the 'sphinx-build' executable. Alternatively you
               # Replace Windows newlines with Unix newlines	echo.may add the Sphinx directory to PATH.
               # Replace Windows newlines with Unix newlines	echo.
               # Replace Windows newlines with Unix newlines	echo.If you don't have Sphinx installed, grab it from
               # Replace Windows newlines with Unix newlines	echo.http://sphinx-doc.org/
               # Replace Windows newlines with Unix newlines	exit /b 1
               # Replace Windows newlines with Unix newlines)
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines%SPHINXBUILD% -M %1 %SOURCEDIR% %BUILDDIR% %SPHINXOPTS% %O%
               # Replace Windows newlines with Unix newlinesgoto end
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines:help
               # Replace Windows newlines with Unix newlines%SPHINXBUILD% -M help %SOURCEDIR% %BUILDDIR% %SPHINXOPTS% %O%
               # Replace Windows newlines with Unix newlines
               # Replace Windows newlines with Unix newlines:end
               # Replace Windows newlines with Unix newlinespopd
               # Replace Windows newlines with Unix newlines