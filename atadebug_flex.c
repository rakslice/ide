
extern int atadebug;

void ATADEBUG(int lvl, char *fmt, int a, int b, int c, int d, int e, int f, int g, int h, int i, int j) {
	if (atadebug < lvl || !fmt)
		return;
	printf(fmt, a, b, c, d, e, f, g, h, i, j);
}
