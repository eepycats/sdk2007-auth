add_rules("mode.release", "mode.debug")
target("steamhook")
	add_defines("UNICODE")	
    set_arch("x86")
    set_kind("shared")
    set_languages("c23")
    set_symbols("debug")

	-- minhook
	add_files("vendor/minhook/src/*.c")
	add_files("vendor/minhook/src/hde/hde32.c")
	add_includedirs("vendor/minhook/include/", "vendor/minhook/src")

    add_files("*.c")
	
    after_build(function (target)
        os.cp(target:targetfile(), "./dist/addons/")
    end)