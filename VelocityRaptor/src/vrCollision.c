#include "..\include\vrCollision.h"
#define GLEW_STATIC
#include <glew.h>
#include <glfw3.h>

inline vrBOOL BiasGreater(const vrFloat a, const vrFloat b)
{
	const vrFloat abs = 0.05;
	const vrFloat rel = 0.95;
	return a >= b * rel + a * abs;
}

void GJKPoly(vrManifold * manifold, const vrPolygonShape A, const vrPolygonShape B)
{
}

void vrPolyPoly(vrManifold* manifold, const vrPolygonShape A, const vrPolygonShape B)
{
	manifold->contact_points = 0;
	vrEdge ref, inc;
	vrVec2 normal;
	vrFloat penetration;
	vrBOOL flip = vrFALSE;
	vrBOOL should_flip = vrFALSE;
	vrVec2 faceA = vrVect(0, 0), faceB = vrVect(0, 0);
	vrEdge edgeA;
	vrEdge edgeB;
	vrFloat penA = vrPolyGetLeastAxis(A, B, &faceA, &edgeA, vrSub(B.center, A.center));
	if (penA <= 0) return;
	vrFloat penB = vrPolyGetLeastAxis(B, A, &faceB, &edgeB, vrSub(A.center, B.center));
	if (penB <= 0) return;

	if (BiasGreater(penA, penB))
	{
		penetration = penB;
		normal = faceB;
		vrVec2 negative_normal = vrVect(-normal.x, -normal.y);

		ref = edgeB;
		inc = vrPolyBestEdge(A, vrPolyGetFarthestVertex(A, negative_normal), negative_normal);
	}
	else
	{
		penetration = penA;
		normal = faceA;
		vrVec2 negative_normal = vrVect(-normal.x, -normal.y);

		ref = edgeA;
		inc = vrPolyBestEdge(B, vrPolyGetFarthestVertex(B, negative_normal), negative_normal);
		flip = vrTRUE;
	}

	/*
	glColor3f(0, 0.5, 1);
	glBegin(GL_LINES);
	glVertex2f(edgeA.a.x, edgeA.a.y);
	glVertex2f(edgeA.b.x, edgeA.b.y);
	glEnd();
	glColor3f(0, 1, 0.5);
	glBegin(GL_LINES);
	glVertex2f(edgeB.a.x, edgeB.a.y);
	glVertex2f(edgeB.b.x, edgeB.b.y);
	glEnd();
	
	
	glColor3f(1, 0, 0);
	glBegin(GL_LINES);
	glVertex2f(ref.a.x, ref.a.y);
	glVertex2f(ref.b.x, ref.b.y);
	glEnd();
	glColor3f(0, 0, 1);
	glBegin(GL_LINES);
	glVertex2f(inc.a.x, inc.a.y);
	glVertex2f(inc.b.x, inc.b.y);
	glEnd();
	*/

	vrVec2 refV = vrNormalize(ref.edge);
	vrVec2 refNorm = vrVect(refV.y, -refV.x);

	should_flip = (vrDot(refNorm, vrSub(B.center, A.center)) >= 0);
	refNorm = should_flip ? vrVect(-refNorm.x, -refNorm.y) : refNorm;
	if (flip) refNorm = vrVect(-refNorm.x, -refNorm.y);

	vrFloat positiveOffset, negativeOffset, Max;
	positiveOffset = vrDot(refV, ref.a);
	negativeOffset = -vrDot(refV, ref.b);
	Max = vrDot(refNorm, ref.max);
	vrVec2 clipped_points[2];
	clipped_points[0] = vrVect(0, 0);
	clipped_points[1] = vrVect(0, 0);


	if (Clip(inc.a, inc.b, refV, positiveOffset, clipped_points) < 2) return;
	if (Clip(clipped_points[0], clipped_points[1], vrVect(-refV.x, -refV.y), negativeOffset, clipped_points) < 2) return;

	manifold->penetration = 0;
	int cp = 0;
	vrFloat separation = vrDot(refNorm, clipped_points[0]) - Max;
	if (separation <= 0.0)
	{
		manifold->contacts[cp].point = clipped_points[0];
		manifold->contacts[cp].normalImpulseSum = 0;
		manifold->contacts[cp].tangentImpulseSum = 0;
		manifold->contacts[cp].depth = separation;
		cp++;
	}
	else
	{
		manifold->penetration = 0;
	}
	separation = vrDot(refNorm, clipped_points[1]) - Max;
	if (separation <= 0.0f)
	{
		manifold->contacts[cp].point = clipped_points[1];
		manifold->contacts[cp].normalImpulseSum = 0;
		manifold->contacts[cp].tangentImpulseSum = 0;
		manifold->contacts[cp].depth = separation;
		manifold->penetration += -separation;
		cp++;
	}

	manifold->penetration /= cp;
	manifold->contact_points = cp;
	manifold->normal = refNorm;
	manifold->flip = flip;
	manifold->penetration = penetration;

	return manifold;
}

void vrPolyCircle(vrManifold * manifold, const vrPolygonShape A, const vrCircleShape B)
{
	manifold->contact_points = 0;

	vrVec2 relPos = vrSub(B.center, A.center);
	vrFloat rsqr = B.radius * B.radius;
	vrVec2 point;

	vrFloat separation = -1000000;
	vrVec2 n;
	vrNode* faceNormal;
	vrEdge normalEdge;

	//
	vrVec2 axis;
	vrEdge nE;
	vrFloat pen2;

	vrNode* current = A.axes->head;
	while (current)
	{
		{
			axis = ((vrVertex*)current->data)->vertex;

			//	axis = vrVect(-axis.x, -axis.y);
			nE = vrPolyBestEdge(A, vrPolyGetFarthestVertex(A, axis), axis);


			vrFloat pen = vrDot(axis, vrSub(B.center, nE.a));
			pen2 = vrDot(axis, vrSub(B.center, nE.b));
			if (pen2 > pen) pen = pen2;

			if (pen > separation)
			{
				separation = pen;
				if (separation > B.radius) return;
				n = axis;
				normalEdge = nE;
			}
		}
		current = current->next;
	}
	//


	manifold->penetration = B.radius - separation;

	vrFloat dot1 = vrDot(vrSub(B.center, normalEdge.a), vrSub(normalEdge.b, normalEdge.a));
	vrFloat dot2 = vrDot(vrSub(B.center, normalEdge.b), vrSub(normalEdge.a, normalEdge.b));

	if (dot1 <= 0)
	{
		if (vrDist_Sqr(B.center, normalEdge.a) > B.radius*B.radius)
			return;

		point = normalEdge.a;
		manifold->normal = vrNormalize(vrSub(B.center, normalEdge.a));
		manifold->penetration = vrDist(B.center, normalEdge.a) - B.radius;
		manifold->penetration = -manifold->penetration;
	}
	else if (dot2 <= 0)
	{
		if (vrDist_Sqr(B.center, normalEdge.b) > B.radius*B.radius)
			return;

		point = normalEdge.b;
		manifold->normal = vrNormalize(vrSub(B.center, normalEdge.b));
		manifold->penetration = vrDist(B.center, normalEdge.b) - B.radius;
		manifold->penetration = -manifold->penetration;
	}
	else
	{
		vrVec2 normal = n;
		vrEdge e = normalEdge;
		vrVec2 a = e.a;
		vrVec2 b = e.b;
		vrVec2 c = B.center;
		vrVec2 d;
		vrVec2 vb = vrNormalize(vrSub(b, a));
		vrFloat dp = vrDot(vrSub(c, a), vb);
		d = vrAdd(a, vrScale(vb, dp));
		vrFloat distance = vrDist(d, c);

		if (distance <= B.radius && dp < vrDist(a, b))
		{
			manifold->normal = n;
			point = d;

		}
		else
		{
			return;
		}
	}

	manifold->contact_points = 1;
	manifold->normal = (vrDot(manifold->normal, vrSub(B.center, A.center)) >= 0) ? vrVect(-manifold->normal.x, -manifold->normal.y) : manifold->normal;

	manifold->contacts[0].point = point;
	manifold->contacts[0].depth = -manifold->penetration;
	manifold->contacts[0].normalImpulseSum = 0;
	manifold->contacts[0].tangentImpulseSum = 0;
	manifold->flip = vrFALSE;
}


void vrCirclePoly(vrManifold * manifold, const vrCircleShape A, const vrPolygonShape B)
{
	vrPolyCircle(manifold, B, A);
	manifold->flip = vrTRUE;
}

void vrCircleCircle(vrManifold * manifold, const vrCircleShape A, const vrCircleShape B)
{
	manifold->normal = vrSub(A.center, B.center);
	vrFloat r = A.radius + B.radius;
	vrFloat nLen = vrLengthSqr(manifold->normal);

	if (nLen > r*r) return;
	nLen = VR_SQRT(nLen);

	manifold->normal = vrScale(manifold->normal, 1.0 / nLen);

	if (nLen == 0.0)
	{
		manifold->contacts[0].point = A.center;
		manifold->contacts[0].depth = -(r - nLen);
		manifold->penetration = -manifold->contacts[0].depth;
		manifold->normal = vrVect(-1, 0);
	}
	else
	{
		manifold->contacts[0].depth = -(r - nLen);
		manifold->contacts[0].point = vrAdd(A.center, vrScale(vrVect(-manifold->normal.x, -manifold->normal.y), A.radius));
		manifold->penetration = -manifold->contacts[0].depth;
	}
	manifold->contact_points = 1;
	manifold->flip = vrFALSE;

}


vrFloat vrPolyGetLeastAxis(const vrPolygonShape a, const vrPolygonShape b, vrVec2 * least_axis, vrEdge* pedge, vrVec2 dir)
{
	vrFloat penetration = 1000000;
	vrNode* vertex = a.vertices->head;
	vrNode* axes = a.axes->head;
	vrEdge edge;
	vrVec2 la;
	while (vertex)
	{
		vrVec2 axis;
		vrVec2 v1 = ((vrVertex*)vertex->data)->vertex;

		vrVec2 v2;
		if (vertex->next == NULL)
			v2 = ((vrVertex*)a.vertices->head->data)->vertex;
		else
			v2 = ((vrVertex*)vertex->next->data)->vertex;

		axis = vrSub(v2, v1);
		axis = vrNormalize(vrVect(axis.y, -axis.x));
		vrVec2 axes_test = ((vrVertex*)axes->data)->vertex;


		if ((vrDot(axes_test, dir) >= 0))
		{
			vrProjection p1 = vrProject(a, axes_test);
			vrProjection p2 = vrProject(b, axes_test);

			vrFloat pen = (VR_MIN(p2.max, p1.max) - VR_MAX(p2.min, p1.min));
			if (pen <= penetration)
			{
				penetration = pen;
				la = axes_test;
				edge.b = v2;
				edge.a = v1;
				edge.edge = vrSub(v2, v1);
				if (vrDot(v2, axes_test) <= vrDot(v1, axes_test))
				{
					edge.max = v2;
					edge.min = v1;
				}
				else
				{
					edge.max = v1;
					edge.min = v2;

				}
			}
		}
		axes = axes->next;
		vertex = vertex->next;
	}
	*pedge = edge;
	*least_axis = la;
	return penetration;

}

vrNode* vrPolyGetFarthestVertex(const vrPolygonShape shape, const vrVec2 normal)
{
	vrNode* f = shape.vertices->head;
	vrVec2 fv = ((vrVertex*)shape.vertices->head->data)->vertex;
	vrFloat max = vrDot(normal, fv);
	vrNode* current = shape.vertices->head->next;
	while (current)
	{
		vrVec2 v = ((vrVertex*)current->data)->vertex;
		vrFloat projection = vrDot(normal, v);
		if (projection > max)
		{
			f = current;
			max = projection;
		}
		current = current->next;
	}
	return f;

}

vrEdge vrPolyBestEdge(const vrPolygonShape shape, vrNode* vert, const vrVec2 normal)
{
	vrEdge edge;
	vrVec2 v = ((vrVertex*)vert->data)->vertex;
	vrVec2 v1;
	vrVec2 v2;
	if (vert->next != NULL)
		v1 = ((vrVertex*)vert->next->data)->vertex;
	else
		v1 = ((vrVertex*)shape.vertices->head->data)->vertex;

	if (vert->prev != NULL)
		v2 = ((vrVertex*)vert->prev->data)->vertex;
	else
		v2 = ((vrVertex*)shape.vertices->head->prev->data)->vertex;
	vrVec2 l = vrSub(v, v1);
	vrVec2 r = vrSub(v, v2);
	vrVec2 le = l;
	vrVec2 re = vrVect(-r.x, -r.y);
	l = vrNormalize(l);
	r = vrNormalize(r);
	if (vrDot(r, normal) <= vrDot(l, normal))
	{
		edge.max = v;
		edge.min = v2;
		edge.a = v;
		edge.b = v2;
		edge.edge = re;
		return edge;
	}
	else
	{
		edge.max = v;
		edge.min = v1;
		edge.a = v1;
		edge.b = v;
		edge.edge = le;
		return edge;
	}
}
vrProjection vrProject(const vrPolygonShape a, const vrVec2 axis)
{
	vrFloat c = vrDot(((vrVertex*)a.vertices->head->data)->vertex, axis);
	//returns the min and the max
	vrProjection p = vrInitProjection(c, c);
	vrNode* vertex = a.vertices->head->next;
	while (vertex)
	{
		c = vrDot(((vrVertex*)vertex->data)->vertex, axis);
		if (c < p.min) p.min = c;
		else if (c > p.max) p.max = c;

		vertex = vertex->next;
	}
	return p;
}

int Clip(const vrVec2 v1, const vrVec2 v2, const vrVec2 n, const vrFloat offset, vrVec2 * clipped_points)
{
	vrFloat d1 = vrDot(n, v1) - offset;
	vrFloat d2 = vrDot(n, v2) - offset;
	int points = 0;
	if (d1 >= 0.0)
	{
		clipped_points[points] = v1;
		points++;
	}
	if (d2 >= 0.0)
	{
		clipped_points[points] = v2;
		points++;
	}

	if (d1 * d2 < 0.0 && points < 2)
	{
		vrFloat u = d1 / (d1 - d2);
		vrVec2 e = vrAdd(v1, vrScale(vrSub(v2, v1), u));
		clipped_points[points] = e;
		points++;
	}
	VR_ASSERT(points >= 3, "Error: Added to many points in clip");

	return points;
}

vrProjection vrInitProjection(vrFloat max, vrFloat min)
{
	vrProjection p;
	p.max = max;
	p.min = min;
	return p;
}