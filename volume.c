#include <u.h>
#include <libc.h>
#include <draw.h>
#include <event.h>

enum {
	SEC = 1000
};

int vfd, volume;
int ismixfs;
Image *knob;
Image *back;
Image *rim;

Point
volumept(Point c, int volume, int r)
{
	double rad;

	rad = (double)(volume*3.0+30) * PI/180.0;
	c.x -= sin(rad) * r;
	c.y += cos(rad) * r;
	return c;
}

void
redraw(void)
{
	Point c;

	c = divpt(addpt(screen->r.min, screen->r.max), 2);
	draw(screen, screen->r, back, nil, ZP);
	line(screen, volumept(c, 0, 45), volumept(c, 0, 40), 0, 0, 1, display->black, ZP);
	line(screen, volumept(c, 100, 45), volumept(c, 100, 40), 0, 0, 1, display->black, ZP);
	ellipse(screen, c, 40, 40, 1, rim, ZP);
	fillellipse(screen, volumept(c, volume, 30), 3, 3, knob, ZP);
}

void
update(void)
{
	char buf[256], *s;
	int n;

	if((n = pread(vfd, buf, sizeof(buf)-1, 0)) <= 0)
		sysfatal("pread: %r");
	buf[n] = 0;
	strtok(buf, " ");
	s = strtok(nil, " ");
	volume = strtol(s, nil, 0);
}

void
eresized(int new)
{
	if(new && getwindow(display, Refnone) < 0)
		fprint(2, "can't reattach to window");
	redraw();
}

void
main()
{
	Mouse m;
	Event e;
	Point p;
	double rad;
	int Etimer, key, d;
	char buf[3];

	vfd = open("/dev/volume", ORDWR);
	if(vfd < 0)
		sysfatal("open: %r");
	read(vfd, buf, sizeof buf);
	ismixfs = strncmp(buf, "mix", 3) == 0;
	update();
	if(initdraw(0, 0, "volume") < 0)
		sysfatal("initdraw: %r");
	back = allocimagemix(display, 0x88FF88FF, DWhite);
	knob = allocimage(display, Rect(0,0,1,1), CMAP8, 1, 0x008800FF);
	rim = allocimage(display, Rect(0,0,1,1), CMAP8, 1, 0x004400FF);
	einit(Emouse);
	Etimer = etimer(0, 2*SEC);
	redraw();
	for(;;){
		key = event(&e);
		if(key == Emouse){
			m = e.mouse;
			if(m.buttons & 1){
				p = subpt(m.xy, divpt(addpt(screen->r.min, screen->r.max), 2));
				rad = atan2(-p.x, p.y);
				d = rad * 180.0/PI;
				if(d < 0)
					d += 360;
				d *= 160.0/360.0;
				if(d < 30)
					d = 0;
				else if(d > 130)
					d = 100;
				else
					d -= 30;
				volume = d;
				fprint(vfd, "%s%d", ismixfs? "mix " : "", volume);
				redraw();
			}
		}else if(key == Etimer){
			update();
			redraw();
		}
	}
}
