#include "primitives.h"
#include <math.h>
#include <string.h>
#include "Wad.h"

tQuad::tQuad(float x, float y, float w, float h)
{
	v1 = tVert(x,   y,   0, 0, 0);
	v2 = tVert(x,   y+h, 0, 0, 1);
	v3 = tVert(x+w, y+h, 0, 1, 1);

	v4 = tVert(x,   y,   0, 0, 0);
	v5 = tVert(x+w, y+h, 0, 1, 1);
	v6 = tVert(x+w, y,   0, 1, 0);
}

tQuad::tQuad(float x, float y, float w, float h, float uu1, float vv1, float uu2, float vv2)
{
	v1 = tVert(x,   y,   0, uu1, vv1);
	v2 = tVert(x,   y+h, 0, uu1, vv2);
	v3 = tVert(x+w, y+h, 0, uu2, vv2);

	v4 = tVert(x,   y,   0, uu1, vv1);
	v5 = tVert(x+w, y+h, 0, uu2, vv2);
	v6 = tVert(x+w, y,   0, uu2, vv1);
}

tQuad::tQuad(tVert v1, tVert v2, tVert v3, tVert v4)
{
	this->v1 = v1;
	this->v2 = v2;
	this->v3 = v3;

	this->v4 = v1;
	this->v5 = v3;
	this->v6 = v4;
}

cQuad::cQuad(cVert v1, cVert v2, cVert v3, cVert v4)
{
	this->v1 = v1;
	this->v2 = v2;
	this->v3 = v3;

	this->v4 = v1;
	this->v5 = v3;
	this->v6 = v4;
}

void cQuad::setColor(COLOR3 c )
{
	v1.c = c;
	v2.c = c;
	v3.c = c;
	v4.c = c;
	v5.c = c;
	v6.c = c;
}

void cQuad::setColor(COLOR3 c1, COLOR3 c2, COLOR3 c3, COLOR3 c4 )
{
	v1.c = c1;
	v2.c = c2;
	v3.c = c3;
	v4.c = c1;
	v5.c = c3;
	v6.c = c4;
}

tCube::tCube(vec3 mins, vec3 maxs)
{
	tVert v1, v2, v3, v4;

	v1 = tVert(mins.x, maxs.y, maxs.z, 0, 0);
	v2 = tVert(mins.x, maxs.y, mins.z, 0, 1);
	v3 = tVert(mins.x, mins.y, mins.z, 1, 1);
	v4 = tVert(mins.x, mins.y, maxs.z, 1, 0);
	left = tQuad(v1, v2, v3, v4);

	v1 = tVert(maxs.x, maxs.y, mins.z, 0, 0);
	v2 = tVert(maxs.x, maxs.y, maxs.z, 0, 1);
	v3 = tVert(maxs.x, mins.y, maxs.z, 1, 1);
	v4 = tVert(maxs.x, mins.y, mins.z, 1, 0);
	right = tQuad(v1, v2, v3, v4);

	v1 = tVert(mins.x, mins.y, mins.z, 0, 0);
	v2 = tVert(maxs.x, mins.y, mins.z, 0, 1);
	v3 = tVert(maxs.x, mins.y, maxs.z, 1, 1);
	v4 = tVert(mins.x, mins.y, maxs.z, 1, 0);
	top = tQuad(v1, v2, v3, v4);

	v1 = tVert(mins.x, maxs.y, maxs.z, 0, 0);
	v2 = tVert(maxs.x, maxs.y, maxs.z, 0, 1);
	v3 = tVert(maxs.x, maxs.y, mins.z, 1, 1);
	v4 = tVert(mins.x, maxs.y, mins.z, 1, 0);
	bottom = tQuad(v1, v2, v3, v4);

	v1 = tVert(mins.x, maxs.y, mins.z, 0, 0);
	v2 = tVert(maxs.x, maxs.y, mins.z, 0, 1);
	v3 = tVert(maxs.x, mins.y, mins.z, 1, 1);
	v4 = tVert(mins.x, mins.y, mins.z, 1, 0);
	front = tQuad(v1, v2, v3, v4);

	v1 = tVert(maxs.x, maxs.y, maxs.z, 0, 0);
	v2 = tVert(mins.x, maxs.y, maxs.z, 0, 1);
	v3 = tVert(mins.x, mins.y, maxs.z, 1, 1);
	v4 = tVert(maxs.x, mins.y, maxs.z, 1, 0);
	back = tQuad(v1, v2, v3, v4);
}

cCube::cCube(vec3 mins, vec3 maxs, COLOR3 c)
{
	cVert v1, v2, v3, v4;

	v1 = cVert(mins.x, maxs.y, maxs.z, c);
	v2 = cVert(mins.x, maxs.y, mins.z, c);
	v3 = cVert(mins.x, mins.y, mins.z, c);
	v4 = cVert(mins.x, mins.y, maxs.z, c);
	left = cQuad(v1, v2, v3, v4);

	v1 = cVert(maxs.x, maxs.y, mins.z, c);
	v2 = cVert(maxs.x, maxs.y, maxs.z, c);
	v3 = cVert(maxs.x, mins.y, maxs.z, c);
	v4 = cVert(maxs.x, mins.y, mins.z, c);
	right = cQuad(v1, v2, v3, v4);

	v1 = cVert(mins.x, mins.y, mins.z, c);
	v2 = cVert(maxs.x, mins.y, mins.z, c);
	v3 = cVert(maxs.x, mins.y, maxs.z, c);
	v4 = cVert(mins.x, mins.y, maxs.z, c);
	top = cQuad(v1, v2, v3, v4);

	v1 = cVert(mins.x, maxs.y, maxs.z, c);
	v2 = cVert(maxs.x, maxs.y, maxs.z, c);
	v3 = cVert(maxs.x, maxs.y, mins.z, c);
	v4 = cVert(mins.x, maxs.y, mins.z, c);
	bottom = cQuad(v1, v2, v3, v4);

	v1 = cVert(mins.x, maxs.y, mins.z, c);
	v2 = cVert(maxs.x, maxs.y, mins.z, c);
	v3 = cVert(maxs.x, mins.y, mins.z, c);
	v4 = cVert(mins.x, mins.y, mins.z, c);
	front = { v1, v2, v3, v4 };

	v1 = cVert(maxs.x, maxs.y, maxs.z, c);
	v2 = cVert(mins.x, maxs.y, maxs.z, c);
	v3 = cVert(mins.x, mins.y, maxs.z, c);
	v4 = cVert(maxs.x, mins.y, maxs.z, c);
	back = cQuad(v1, v2, v3, v4);
}

void cCube::setColor(COLOR3 c )
{
	left.setColor(c);
	right.setColor(c);
	top.setColor(c);
	bottom.setColor(c);
	front.setColor(c);
	back.setColor(c);
}

void cCube::setColor(COLOR3 lf, COLOR3 rt, COLOR3 tp, COLOR3 bt, COLOR3 ft, COLOR3 bk )
{
	left.setColor(lf);
	right.setColor(rt);
	top.setColor(tp);
	bottom.setColor(bt);
	front.setColor(ft);
	back.setColor(bk);
}

