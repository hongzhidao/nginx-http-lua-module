cat << END                                            >> $NGX_MAKEFILE

lua=$ngx_addon_dir/lua-5.4.6

$lua/src/liblua.a:
	cd $lua \\
	&& if [ -f src/liblua.a ]; then \$(MAKE) clean; fi \\
	&& \$(MAKE)

END
