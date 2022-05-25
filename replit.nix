{ pkgs }: {
	deps = [
		pkgs.clang_12
		pkgs.ccls
		pkgs.gdb
		pkgs.gnumake
		pkgs.valgrind
		pkgs.less
		pkgs.htop
		pkgs.zip
		pkgs.python38
	];
}