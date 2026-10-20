#include "common_types.h"
#include <math.h>

/* The math of CoD2rev-Plus's com_math.cpp that the bgame's movement calls and
 * whose results the engine's own versions do not match bit for bit (the
 * server's AngleDelta rounds its difference to a float before quantizing it,
 * vectoyaw and vectoangles round through doubles, PitchForYawOnNormal goes
 * through YawVectors and vectopitch). bg_plus_compat.h sends the bgame's
 * calls here; the rest of the engine keeps its own. Separate functions, so
 * that each argument is rounded to a float at the call as in the server. */

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define BGP_YAW 1
#define BGP_PITCH 0
#define BGP_ROLL 2

static float BGP_I_sqrt(const float value)
{
	return sqrt(value);
}

float BGP_AngleNormalize360(float angle)
{
	return ( 360.0 / 65536 ) * ( (int)( angle * ( 65536 / 360.0 ) ) & 65535 );
}

float BGP_AngleNormalize180(float angle)
{
	angle = BGP_AngleNormalize360( angle );

	if ( angle > 180.0 )
	{
		angle -= 360.0;
	}

	return angle;
}

float BGP_AngleDelta(float angle1, float angle2)
{
	return BGP_AngleNormalize180( angle1 - angle2 );
}

void BGP_AngleVectors(const vec_t *angles, vec_t *forward, vec_t *right, vec_t *up)
{
	float angle;
	static float sr, sp, sy, cr, cp, cy;

	angle = angles[BGP_YAW] * ( M_PI * 2 / 360 );
	sy = sin( angle );
	cy = cos( angle );

	angle = angles[BGP_PITCH] * ( M_PI * 2 / 360 );
	sp = sin( angle );
	cp = cos( angle );

	angle = angles[BGP_ROLL] * ( M_PI * 2 / 360 );
	sr = sin( angle );
	cr = cos( angle );

	if ( forward )
	{
		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;
	}

	if ( right )
	{
		right[0] = ( -1 * sr * sp * cy + - 1 * cr * -sy );
		right[1] = ( -1 * sr * sp * sy + - 1 * cr * cy );
		right[2] = -1 * sr * cp;
	}

	if ( up )
	{
		up[0] = ( cr * sp * cy + - sr * -sy );
		up[1] = ( cr * sp * sy + - sr * cy );
		up[2] = cr * cp;
	}
}

float BGP_vectoyaw(const vec_t *vec)
{
	float yaw;

	if ( vec[BGP_YAW] == 0 && vec[BGP_PITCH] == 0 )
	{
		return 0.0;
	}

	yaw = ( atan2( vec[BGP_YAW], vec[BGP_PITCH] ) * 180 / M_PI );

	if(yaw >= 0.0)
	{
		return yaw;
	}

	return yaw + 360;
}

void BGP_vectoangles(const vec_t *value1, vec_t *angles)
{
	float forward;
	float yaw, pitch;

	if ( value1[1] == 0 && value1[0] == 0 )
	{
		yaw = 0;
		if ( value1[2] > 0 )
		{
			pitch = 270;
		}
		else
		{
			pitch = 90;
		}
	}
	else
	{
		yaw = ( atan2( value1[1], value1[0] ) * 180 / M_PI );
		if(yaw < 0.0)
		{
			yaw += 360.0;
		}
		forward = BGP_I_sqrt( value1[0] * value1[0] + value1[1] * value1[1] );
		pitch = ( atan2( value1[2], forward ) * -180 / M_PI );
		if(pitch < 0.0)
		{
			pitch += 360.0;
		}

	}

	angles[BGP_PITCH] = pitch;
	angles[BGP_YAW] = yaw;
	angles[BGP_ROLL] = 0;
}

static void BGP_YawVectors(const float yaw, vec_t *forward, vec_t *right)
{
	float cy;
	float angle;
	float sy;

	angle = yaw * 0.017453292;
	cy = cos(angle);
	sy = sin(angle);

	if ( forward )
	{
		forward[0] = cy;
		forward[1] = sy;
		forward[2] = 0.0;
	}

	if ( right )
	{
		right[0] = sy;
		right[1] = -cy;
		right[2] = 0.0;
	}
}

static float BGP_vectopitch(const vec_t *vec)
{
	float forward;
	float at;
	float a;

	if ( 0.0 != vec[1] || 0.0 != vec[0] )
	{
		at = atan2(vec[2], BGP_I_sqrt(vec[1] * vec[1] + vec[0] * vec[0]));
		a = at * -180.0 / M_PI;
		if ( a < 0.0 )
		{
			forward = a + 360.0;
		}
		else
		{
			forward = a + 0.0;
		}
	}
	else if ( -vec[2] < 0.0 )
	{
		forward = 270.0;
	}
	else
	{
		forward = 90.0;
	}
	return forward;
}

extern void ProjectPointOnPlane(const vec_t *p, const vec_t *normal, vec_t *dst);

float BGP_PitchForYawOnNormal(const float fYaw, const vec_t *normal)
{
	vec3_t dst;
	vec3_t forward;

	BGP_YawVectors(fYaw, forward, 0);
	ProjectPointOnPlane(forward, normal, dst);

	return BGP_vectopitch(dst);
}
