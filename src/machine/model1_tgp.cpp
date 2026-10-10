#include "model1.h"
// license:BSD-3-Clause
// copyright-holders:Olivier Galibert
/*
** Model 1 coprocessor TGP simulation
*/



void model1::fifoout_push(u32 data)
{
	m_copro_fifo_out->push(u32(data));
}

void model1::fifoout_push_f(float data)
{
	m_copro_fifo_out->push(f2u(data));
}

u32 model1::fifoin_pop()
{
	return m_copro_fifo_in->pop();
}

float model1::fifoin_pop_f()
{
	return u2f(m_copro_fifo_in->pop());
}

float model1::tcos(s16 a)
{
	if(a == 16384 || a == -16384)
		return 0;
	else if(a == -32768)
		return -1;
	else if(a == 0)
		return 1;
	else
		return cos(a*(2*M_PI/65536.0));
}

float model1::tsin(s16 a)
{
	if(a == 0 || a == -32768)
		return 0;
	else if(a == 16384)
		return 1;
	else if(a == -16384)
		return -1;
	else
		return sin(a*(2*M_PI/65536.0));
}

u16 model1::ram_get_i()
{
	return m_copro_ram_data[(m_copro_hle_ram_scan_adr++) & 0x7fff];
}

float model1::ram_get_f()
{
	return u2f(m_copro_ram_data[(m_copro_hle_ram_scan_adr++) & 0x7fff]);
}

TGP_FUNCTION( model1::fadd )
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float r = a+b;
	M1_LOG(0, "TGP fadd %f+%f=%f (%x)\n", a, b, r, m_pushpc);
	fifoout_push_f(r);
}

TGP_FUNCTION( model1::fsub )
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float r = a-b;
	M1_LOG(0, "TGP fsub %f-%f=%f (%x)\n", a, b, r, m_pushpc);
	fifoout_push_f(r);
}

TGP_FUNCTION( model1::fmul )
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float r = a*b;
	M1_LOG(0, "TGP fmul %f*%f=%f (%x)\n", a, b, r, m_pushpc);
	fifoout_push_f(r);
}

TGP_FUNCTION( model1::fdiv )
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
//  float r = !b ? 1e39 : a/b;
	float r = !b ? 0 : a * (1/b);
	M1_LOG(0, "TGP fdiv %f/%f=%f (%x)\n", a, b, r, m_pushpc);
	fifoout_push_f(r);
}

TGP_FUNCTION( model1::matrix_push )
{
	if(m_mat_stack_pos != MAT_STACK_SIZE) {
		memcpy(m_mat_stack[m_mat_stack_pos], m_cmat, sizeof(m_cmat));
		m_mat_stack_pos++;
	}
	M1_LOG(0, "TGP matrix_push (depth=%d, pc=%x)\n", m_mat_stack_pos, m_pushpc);
}

TGP_FUNCTION( model1::matrix_pop )
{
	if(m_mat_stack_pos) {
		m_mat_stack_pos--;
		memcpy(m_cmat, m_mat_stack[m_mat_stack_pos], sizeof(m_cmat));
	}
	M1_LOG(0, "TGP matrix_pop (depth=%d, pc=%x)\n", m_mat_stack_pos, m_pushpc);
}

TGP_FUNCTION( model1::matrix_write )
{
	int i;
	for(i=0; i<12; i++)
		m_cmat[i] = fifoin_pop_f();
	M1_LOG(0, "TGP matrix_write %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f) (%x)\n",
				m_cmat[0], m_cmat[1], m_cmat[2], m_cmat[3], m_cmat[4], m_cmat[5], m_cmat[6], m_cmat[7], m_cmat[8], m_cmat[9], m_cmat[10], m_cmat[11],
				m_pushpc);
}

TGP_FUNCTION( model1::clear_stack )
{
	M1_LOG(0, "TGP clear_stack (%x)\n", m_pushpc);
	m_mat_stack_pos = 0;
}

TGP_FUNCTION( model1::matrix_mul )
{
	float m[12];
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	float e = fifoin_pop_f();
	float f = fifoin_pop_f();
	float g = fifoin_pop_f();
	float h = fifoin_pop_f();
	float i = fifoin_pop_f();
	float j = fifoin_pop_f();
	float k = fifoin_pop_f();
	float l = fifoin_pop_f();
	M1_LOG(0, "TGP matrix_mul %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f (%x)\n", a, b, c, d, e, f, g, h, i, j, k, l, m_pushpc);
	m[0]  = a*m_cmat[0] + b*m_cmat[3] + c*m_cmat[6];
	m[1]  = a*m_cmat[1] + b*m_cmat[4] + c*m_cmat[7];
	m[2]  = a*m_cmat[2] + b*m_cmat[5] + c*m_cmat[8];
	m[3]  = d*m_cmat[0] + e*m_cmat[3] + f*m_cmat[6];
	m[4]  = d*m_cmat[1] + e*m_cmat[4] + f*m_cmat[7];
	m[5]  = d*m_cmat[2] + e*m_cmat[5] + f*m_cmat[8];
	m[6]  = g*m_cmat[0] + h*m_cmat[3] + i*m_cmat[6];
	m[7]  = g*m_cmat[1] + h*m_cmat[4] + i*m_cmat[7];
	m[8]  = g*m_cmat[2] + h*m_cmat[5] + i*m_cmat[8];
	m[9]  = j*m_cmat[0] + k*m_cmat[3] + l*m_cmat[6] + m_cmat[9];
	m[10] = j*m_cmat[1] + k*m_cmat[4] + l*m_cmat[7] + m_cmat[10];
	m[11] = j*m_cmat[2] + k*m_cmat[5] + l*m_cmat[8] + m_cmat[11];

	memcpy(m_cmat, m, sizeof(m));
}

TGP_FUNCTION( model1::anglev )
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	M1_LOG(0, "TGP anglev %f, %f (%x)\n", a, b, m_pushpc);
	if(!b) {
		if(a>=0)
			fifoout_push(0);
		else
			fifoout_push((u32)-32768);
	} else if(!a) {
		if(b>=0)
			fifoout_push(16384);
		else
			fifoout_push((u32)-16384);
	} else
		fifoout_push((s16)(atan2(b, a)*32768/M_PI));
}

TGP_FUNCTION( model1::triangle_normal )
{
	float p1x = fifoin_pop_f();
	float p1y = fifoin_pop_f();
	float p1z = fifoin_pop_f();
	float p2x = fifoin_pop_f();
	float p2y = fifoin_pop_f();
	float p2z = fifoin_pop_f();
	float p3x = fifoin_pop_f();
	float p3y = fifoin_pop_f();
	float p3z = fifoin_pop_f();

	float nx = (p1y-p2y)*(p3z-p2z) - (p3y-p2y)*(p1z-p2z);
	float ny = (p1z-p2z)*(p3x-p2x) - (p3z-p2z)*(p1x-p2x);
	float nz = (p1x-p2x)*(p3y-p2y) - (p3x-p2x)*(p1y-p2y);

	float nn = sqrtf(nx*nx + ny*ny + nz*nz);
	if(nn < 1e-3)
		nn = 0;
	else
		nn = 1/nn;

	nx *= nn;
	ny *= nn;
	nz *= nn;

	M1_LOG(0, "TGP triangle_normal %f, %f, %f, %f, %f, %f, %f, %f, %f (%x)\n", p1x, p1y, p1z, p2x, p2y,p2z, p3x, p3y, p3z, m_pushpc);
	fifoout_push_f(nx);
	fifoout_push_f(ny);
	fifoout_push_f(nz);
}

TGP_FUNCTION( model1::normalize )
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float n = (a*a+b*b+c*c) / sqrtf(a*a+b*b+c*c);
	M1_LOG(0, "TGP normalize %f, %f, %f (%x)\n", a, b, c, m_pushpc);
	fifoout_push_f(a/n);
	fifoout_push_f(b/n);
	fifoout_push_f(c/n);
}

TGP_FUNCTION( model1::acc_seti )
{
	int32_t a = fifoin_pop();
	M1_LOG(0, "TGP acc_seti %d (%x)\n", a, m_pushpc);
	m_acc = a;
}

TGP_FUNCTION( model1::track_select )
{
	int32_t a = fifoin_pop();
	M1_LOG(0, "TGP track_select %d (%x)\n", a, m_pushpc);
	m_tgp_vr_select = a;
}

TGP_FUNCTION( model1::load_base )
{
	m_tgp_vr_base[0] = fifoin_pop_f();
	m_tgp_vr_base[1] = fifoin_pop_f();
	m_tgp_vr_base[2] = fifoin_pop_f();
	m_tgp_vr_base[3] = fifoin_pop_f();

	M1_LOG(0, "TGP load_base %f, %f, %f, %f (%x)\n", m_tgp_vr_base[0], m_tgp_vr_base[1], m_tgp_vr_base[2], m_tgp_vr_base[3], m_pushpc);
}

TGP_FUNCTION( model1::transpose )
{
	M1_LOG(0, "TGP transpose (%x)\n", m_pushpc);

	float t;
	t = m_cmat[1]; m_cmat[1] = m_cmat[3]; m_cmat[3] = t;
	t = m_cmat[2]; m_cmat[2] = m_cmat[6]; m_cmat[6] = t;
	t = m_cmat[5]; m_cmat[5] = m_cmat[7]; m_cmat[7] = t;
}

TGP_FUNCTION( model1::anglep )
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	M1_LOG(0, "TGP anglep %f, %f, %f, %f (%x)\n", a, b, c, d, m_pushpc);
	c = a - c;
	d = b - d;
	if(!d) {
		if(c>=0)
			fifoout_push(0);
		else
			fifoout_push((u32)-32768);
	} else if(!c) {
		if(d>=0)
			fifoout_push(16384);
		else
			fifoout_push((u32)-16384);
	} else
		fifoout_push((s16)(atan2(d, c)*32768/M_PI));
}

TGP_FUNCTION( model1::matrix_ident )
{
	M1_LOG(0, "TGP matrix_ident (%x)\n", m_pushpc);
	memset(m_cmat, 0, sizeof(m_cmat));
	m_cmat[0] = 1.0;
	m_cmat[4] = 1.0;
	m_cmat[8] = 1.0;
}

TGP_FUNCTION( model1::matrix_read )
{
	int i;
	M1_LOG(0, "TGP matrix_read (%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f) (%x)\n",
				m_cmat[0], m_cmat[1], m_cmat[2], m_cmat[3], m_cmat[4], m_cmat[5], m_cmat[6], m_cmat[7], m_cmat[8], m_cmat[9], m_cmat[10], m_cmat[11], m_pushpc);
	for(i=0; i<12; i++)
		fifoout_push_f(m_cmat[i]);
}

TGP_FUNCTION( model1::matrix_trans )
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();

	M1_LOG(0, "TCP matrix_trans %f %f %f (%x)\n", a, b, c, m_pushpc);

	m_cmat[ 9] += m_cmat[0]*a+m_cmat[3]*b+m_cmat[6]*c;
	m_cmat[10] += m_cmat[1]*a+m_cmat[4]*b+m_cmat[7]*c;
	m_cmat[11] += m_cmat[2]*a+m_cmat[5]*b+m_cmat[8]*c;
}

TGP_FUNCTION( model1::matrix_scale )
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	M1_LOG(0, "TGP matrix_scale %f, %f, %f (%x)\n", a, b, c, m_pushpc);
	m_cmat[0] *= a;
	m_cmat[1] *= a;
	m_cmat[2] *= a;
	m_cmat[3] *= b;
	m_cmat[4] *= b;
	m_cmat[5] *= b;
	m_cmat[6] *= c;
	m_cmat[7] *= c;
	m_cmat[8] *= c;
}

TGP_FUNCTION( model1::matrix_rotx )
{
	s16 a = fifoin_pop();
	float s = tsin(a);
	float c = tcos(a);
	float t1, t2;
	M1_LOG(0, "TGP matrix_rotx %d (%x)\n", a, m_pushpc);
	t1 = m_cmat[3];
	t2 = m_cmat[6];
	m_cmat[3] = c*t1-s*t2;
	m_cmat[6] = s*t1+c*t2;
	t1 = m_cmat[4];
	t2 = m_cmat[7];
	m_cmat[4] = c*t1-s*t2;
	m_cmat[7] = s*t1+c*t2;
	t1 = m_cmat[5];
	t2 = m_cmat[8];
	m_cmat[5] = c*t1-s*t2;
	m_cmat[8] = s*t1+c*t2;
}

TGP_FUNCTION( model1::matrix_roty )
{
	s16 a = fifoin_pop();
	float s = tsin(a);
	float c = tcos(a);
	float t1, t2;

	M1_LOG(0, "TGP matrix_roty %d (%x)\n", a, m_pushpc);
	t1 = m_cmat[6];
	t2 = m_cmat[0];
	m_cmat[6] = c*t1-s*t2;
	m_cmat[0] = s*t1+c*t2;
	t1 = m_cmat[7];
	t2 = m_cmat[1];
	m_cmat[7] = c*t1-s*t2;
	m_cmat[1] = s*t1+c*t2;
	t1 = m_cmat[8];
	t2 = m_cmat[2];
	m_cmat[8] = c*t1-s*t2;
	m_cmat[2] = s*t1+c*t2;
}

TGP_FUNCTION( model1::matrix_rotz )
{
	s16 a = fifoin_pop();
	float s = tsin(a);
	float c = tcos(a);
	float t1, t2;

	M1_LOG(0, "TGP matrix_rotz %d (%x)\n", a, m_pushpc);
	t1 = m_cmat[0];
	t2 = m_cmat[3];
	m_cmat[0] = c*t1-s*t2;
	m_cmat[3] = s*t1+c*t2;
	t1 = m_cmat[1];
	t2 = m_cmat[4];
	m_cmat[1] = c*t1-s*t2;
	m_cmat[4] = s*t1+c*t2;
	t1 = m_cmat[2];
	t2 = m_cmat[5];
	m_cmat[2] = c*t1-s*t2;
	m_cmat[5] = s*t1+c*t2;
}

TGP_FUNCTION( model1::track_read_quad )
{
	u32 a = fifoin_pop();
	int offd;

	M1_LOG(0, "TGP track_read_quad %d (%x)\n", a, m_pushpc);

	offd = m_copro_data->as_u32(0x20+m_tgp_vr_select) + 16*a;
	fifoout_push(m_copro_data->as_u32(offd));
	fifoout_push(m_copro_data->as_u32(offd+1));
	fifoout_push(m_copro_data->as_u32(offd+2));
	fifoout_push(m_copro_data->as_u32(offd+3));
	fifoout_push(m_copro_data->as_u32(offd+4));
	fifoout_push(m_copro_data->as_u32(offd+5));
	fifoout_push(m_copro_data->as_u32(offd+6));
	fifoout_push(m_copro_data->as_u32(offd+7));
	fifoout_push(m_copro_data->as_u32(offd+8));
	fifoout_push(m_copro_data->as_u32(offd+9));
	fifoout_push(m_copro_data->as_u32(offd+10));
	fifoout_push(m_copro_data->as_u32(offd+11));
}

TGP_FUNCTION( model1::intercept )
{
	float x1 = fifoin_pop_f();
	float y1 = fifoin_pop_f();
	float z1 = fifoin_pop_f();
	float x2 = fifoin_pop_f();
	float y2 = fifoin_pop_f();
	float z2 = fifoin_pop_f();
	u32 idx = fifoin_pop();

	M1_LOG(0, "TGP intercept %f, %f, %f, %f, %f, %f, %x (%x)\n", x1, y1, z1, x2, y2, z2, idx, m_pushpc);

	float dx = x2-x1;
	float dy = y2-y1;
	float dz = z2-z1;

	idx = m_copro_data->as_u32(0x10) + 2*idx;
	u32 count = m_copro_data->as_u32(idx);
	u32 adr = m_copro_data->as_u32(idx+1);
	u32 ret = 1;

	for(unsigned int j=0; j<count; j++) {
		float point[4][3];
		for(int pt=0; pt<4; pt++)
			for(int dim=0; dim<3; dim++)
				point[pt][dim] = u2f(m_copro_data->as_u32(adr++));
		float plane[4];
		for(int dim=0; dim<4; dim++)
			plane[dim] = u2f(m_copro_data->as_u32(adr++));
		adr++; // 0, 1 or 2...

		float den = dx * plane[0] + dy * plane[1] + dz * plane[2];
		if(den > -0.0001 && den < 0.0001)
			continue;
		float t = - (x1 * plane[0] + y1 * plane[1] + z1 * plane[2] + plane[3]) / den;
		if(t < 0 || t > 1)
			continue;

		float ix = x1 + dx*t;
		float iy = y1 + dy*t;
		float iz = z1 + dz*t;

		int cp = 0;
		for(int pt=0; pt<4; pt++) {
			int pt1 = (pt+1) & 3;
			float p01x = point[pt1][0] - point[pt][0];
			float p01y = point[pt1][1] - point[pt][1];
			float p01z = point[pt1][2] - point[pt][2];
			float p0ix = ix - point[pt][0];
			float p0iy = iy - point[pt][1];
			float p0iz = iz - point[pt][2];
			float det = plane[0] * (p01y * p0iz - p01z * p0iy) + plane[1] * (p01z * p0ix - p01x * p0iz) + plane[2] * (p01x * p0iy - p01y * p0ix);
			cp += det >= 0;
		}
		if(cp == 0 || cp == 4) {
			m_tgp_int_px = ix;
			m_tgp_int_py = iy;
			m_tgp_int_pz = iz;
			ret = 0;
			adr -= 17;
			break;
		}
	}

	m_tgp_int_adr = adr;
	fifoout_push(ret);
}

TGP_FUNCTION( model1::transform_point )
{
	float x = fifoin_pop_f();
	float y = fifoin_pop_f();
	float z = fifoin_pop_f();
	M1_LOG(0, "TGP transform_point %f, %f, %f -> %f, %f, %f (%x)\n", x, y, z,
			 m_cmat[0]*x+m_cmat[3]*y+m_cmat[6]*z+m_cmat[9],
			 m_cmat[1]*x+m_cmat[4]*y+m_cmat[7]*z+m_cmat[10],
			 m_cmat[2]*x+m_cmat[5]*y+m_cmat[8]*z+m_cmat[11],
			 m_pushpc);
	fifoout_push_f(m_cmat[0]*x+m_cmat[3]*y+m_cmat[6]*z+m_cmat[9]);
	fifoout_push_f(m_cmat[1]*x+m_cmat[4]*y+m_cmat[7]*z+m_cmat[10]);
	fifoout_push_f(m_cmat[2]*x+m_cmat[5]*y+m_cmat[8]*z+m_cmat[11]);
}

TGP_FUNCTION( model1::fcos_m1 )
{
	s16 a = fifoin_pop();
	M1_LOG(0, "TGP fcos %d (%x)\n", a, m_pushpc);
	fifoout_push_f(tcos(a));
}

TGP_FUNCTION( model1::fsin_m1 )
{
	s16 a = fifoin_pop();
	M1_LOG(0, "TGP fsin %d (%x)\n", a, m_pushpc);
	fifoout_push_f(tsin(a));
}

TGP_FUNCTION( model1::fcosm_m1 )
{
	s16 a = fifoin_pop();
	float b = fifoin_pop_f();
	M1_LOG(0, "TGP fcosm %d, %f (%x)\n", a, b, m_pushpc);
	fifoout_push_f(b*tcos(a));
}

TGP_FUNCTION( model1::fsinm_m1 )
{
	s16 a = fifoin_pop();
	float b = fifoin_pop_f();
	M1_LOG(0, "TGP fsinm %d, %f (%x)\n", a, b, m_pushpc);
	fifoout_push_f(b*tsin(a));
}

TGP_FUNCTION( model1::distance3 )
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	float e = fifoin_pop_f();
	float f = fifoin_pop_f();
	M1_LOG(0, "TGP distance3 (%f, %f, %f), (%f, %f, %f) (%x)\n", a, b, c, d, e, f, m_pushpc);
	a -= d;
	b -= e;
	c -= f;
	fifoout_push_f(sqrtf(a*a+b*b+c*c));
}

TGP_FUNCTION( model1::ftoi )
{
	float a = fifoin_pop_f();
	M1_LOG(0, "TGP ftoi %f (%x)\n", a, m_pushpc);
	fifoout_push((int)a);
}

TGP_FUNCTION( model1::itof )
{
	int32_t a = fifoin_pop();
	M1_LOG(0, "TGP itof %d (%x)\n", a, m_pushpc);
	fifoout_push_f(a);
}

TGP_FUNCTION( model1::acc_set )
{
	float a = fifoin_pop_f();
	M1_LOG(0, "TGP acc_set %f (%x)\n", a, m_pushpc);
	m_acc = a;
}

TGP_FUNCTION( model1::acc_get )
{
	M1_LOG(0, "TGP acc_get (%x)\n", m_pushpc);
	fifoout_push_f(m_acc);
}

TGP_FUNCTION( model1::acc_add )
{
	float a = fifoin_pop_f();
	M1_LOG(0, "TGP acc_add %f (%x)\n", a, m_pushpc);
	m_acc += a;
}

TGP_FUNCTION( model1::acc_sub )
{
	float a = fifoin_pop_f();
	M1_LOG(0, "TGP acc_sub %f (%x)\n", a, m_pushpc);
	m_acc -= a;
}

TGP_FUNCTION( model1::acc_mul )
{
	float a = fifoin_pop_f();
	M1_LOG(0, "TGP acc_mul %f (%x)\n", a, m_pushpc);
	m_acc *= a;
}

TGP_FUNCTION( model1::acc_div )
{
	float a = fifoin_pop_f();
	M1_LOG(0, "TGP acc_div %f (%x)\n", a, m_pushpc);
	m_acc /= a;
}

TGP_FUNCTION( model1::f42 )
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	(void)a;
	(void)b;
	(void)c;
	M1_LOG(0, "TGP f42 %f, %f, %f (%x)\n", a, b, c, m_pushpc);
	//  fifoout_push_f((m_rand() % 1000) - 500);
	fifoout_push_f(0);
	fifoout_push_f(0);
	fifoout_push_f(0);
	fifoout_push_f(0);
	fifoout_push_f(0);
	fifoout_push_f(0);
}


// r = (x2 + y2 + z2)1/2,     f = tan-1(y/(x2+z2)1/2),     q = tan-1(z/x)

TGP_FUNCTION( model1::xyz2rqf )
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float norm;
	M1_LOG(0, "TGP xyz2rqf %f, %f, %f (%x)\n", a, b, c, m_pushpc);
	fifoout_push_f(sqrtf(a*a+b*b+c*c));
	norm = sqrtf(a*a+c*c);
	if(!c) {
		if(a>=0)
			fifoout_push(0);
		else
			fifoout_push((u32)-32768);
	} else if(!a) {
		if(c>=0)
			fifoout_push(16384);
		else
			fifoout_push((u32)-16384);
	} else
		fifoout_push((s16)(atan2(c, a)*32768/M_PI));

	if(!b)
		fifoout_push(0);
	else if(!norm) {
		if(b>=0)
			fifoout_push(16384);
		else
			fifoout_push((u32)-16384);
	} else
		fifoout_push((s16)(atan2(b, norm)*32768/M_PI));
}

TGP_FUNCTION( model1::f43 )
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	float e = fifoin_pop_f();
	float f = fifoin_pop_f();
	(void)a;
	(void)b;
	(void)c;
	(void)d;
	(void)e;
	(void)f;
	M1_LOG(0, "TGP f43 %f, %f, %f, %f, %f, %f (%x)\n", a, b, c, d, e, f, m_pushpc);
	fifoout_push_f(0);
	fifoout_push_f(0);
	fifoout_push_f(0);
	fifoout_push_f(0);
}

TGP_FUNCTION( model1::f43_swa )
{
	float a = fifoin_pop_f();
	int b = fifoin_pop();
	int c = fifoin_pop();
	(void)a;
	(void)b;
	(void)c;
	M1_LOG(0, "TGP f43_swa %f, %d, %d (%x)\n", a, b, c, m_pushpc);
	fifoout_push_f(0);
	fifoout_push_f(0);
	fifoout_push_f(0);
}

TGP_FUNCTION( model1::track_read_tri )
{
	u32 a = fifoin_pop();
	int offd;

	M1_LOG(0, "TGP track_read_tri %d (%x)\n", a, m_pushpc);

	offd = m_copro_data->as_u32(0x20+m_tgp_vr_select) + 16*a;
	fifoout_push(m_copro_data->as_u32(offd+12));
	fifoout_push(m_copro_data->as_u32(offd+13));
	fifoout_push(m_copro_data->as_u32(offd+14));
}

TGP_FUNCTION( model1::matrix_sdir )
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float norm = sqrtf(a*a+b*b+c*c);
	float t[9], m[9];
	M1_LOG(0, "TGP matrix_sdir %f, %f, %f (%x)\n", a, b, c, m_pushpc);

	memset(t, 0, sizeof(t));

	if(!norm) {
		memset(t, 0, 9*sizeof(t[0]));
		t[0] = 1.0;
		t[4] = 1.0;
		t[8] = 1.0;
	} else {
		t[0] = -c / norm;
		t[1] = b / norm;
		t[2] = a / norm;

		norm = sqrtf(a*a+c*c);
		t[6] = a/norm;
		t[7] = 0;
		t[8] = c/norm;

		t[3] = -b*c;
		t[4] = a*a+c*c;
		t[5] = -b*a;
		norm = sqrtf(t[3]*t[3]+t[4]*t[4]+t[5]*t[5]);
		t[3] /= norm;
		t[4] /= norm;
		t[5] /= norm;
	}

	m[0]  = t[0]*m_cmat[0] + t[1]*m_cmat[3] + t[2]*m_cmat[6];
	m[1]  = t[0]*m_cmat[1] + t[1]*m_cmat[4] + t[2]*m_cmat[7];
	m[2]  = t[0]*m_cmat[2] + t[1]*m_cmat[5] + t[2]*m_cmat[8];
	m[3]  = t[3]*m_cmat[0] + t[4]*m_cmat[3] + t[5]*m_cmat[6];
	m[4]  = t[3]*m_cmat[1] + t[4]*m_cmat[4] + t[5]*m_cmat[7];
	m[5]  = t[3]*m_cmat[2] + t[4]*m_cmat[5] + t[5]*m_cmat[8];
	m[6]  = t[6]*m_cmat[0] + t[7]*m_cmat[3] + t[8]*m_cmat[6];
	m[7]  = t[6]*m_cmat[1] + t[7]*m_cmat[4] + t[8]*m_cmat[7];
	m[8]  = t[6]*m_cmat[2] + t[7]*m_cmat[5] + t[8]*m_cmat[8];

	memcpy(m_cmat, m, sizeof(m));
}

TGP_FUNCTION( model1::fsqrt )
{
	float a = fifoin_pop_f();
	M1_LOG(0, "TGP fsqrt %f (%x)\n", a, m_pushpc);
	fifoout_push_f(sqrtf(a));
}

TGP_FUNCTION( model1::vlength )
{
	float x = fifoin_pop_f();
	float y = fifoin_pop_f();
	float z = fifoin_pop_f();
	M1_LOG(0, "TGP vlength %f, %f, %f (%x)\n", x, y, z, m_pushpc);

	x -= m_tgp_vr_base[0];
	y -= m_tgp_vr_base[1];
	z -= m_tgp_vr_base[2];
	fifoout_push_f(sqrtf(x*x + y*y + z*z) - m_tgp_vr_base[3]);
}

TGP_FUNCTION( model1::f47 )
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	M1_LOG(0, "TGP f47 %f, %f, %f (%x)\n", a, b, c, m_pushpc);
	fifoout_push_f(a+c);
	fifoout_push_f(b+c);
}

TGP_FUNCTION( model1::track_read_info )
{
	u16 a = fifoin_pop();
	int offd;

	M1_LOG(0, "TGP track_read_info %d (%x)\n", a, m_pushpc);

	offd = m_copro_data->as_u32(0x20+m_tgp_vr_select) + 16*a;
	fifoout_push(m_copro_data->as_u32(offd+15));
}

TGP_FUNCTION( model1::colbox_set )
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	float e = fifoin_pop_f();
	float f = fifoin_pop_f();
	float g = fifoin_pop_f();
	float h = fifoin_pop_f();
	float i = fifoin_pop_f();
	float j = fifoin_pop_f();
	float k = fifoin_pop_f();
	float l = fifoin_pop_f();
	M1_LOG(0, "TGP colbox_set %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f (%x)\n", a, b, c, d, e, f, g, h, i, j, k, l, m_pushpc);
	m_tgp_vr_cbox[ 0] = a;
	m_tgp_vr_cbox[ 1] = b;
	m_tgp_vr_cbox[ 2] = c;
	m_tgp_vr_cbox[ 3] = d;
	m_tgp_vr_cbox[ 4] = e;
	m_tgp_vr_cbox[ 5] = f;
	m_tgp_vr_cbox[ 6] = g;
	m_tgp_vr_cbox[ 7] = h;
	m_tgp_vr_cbox[ 8] = i;
	m_tgp_vr_cbox[ 9] = j;
	m_tgp_vr_cbox[10] = k;
	m_tgp_vr_cbox[11] = l;
}

TGP_FUNCTION( model1::colbox_test )
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	(void)a;
	(void)b;
	(void)c;
	M1_LOG(0, "TGP colbox_test %f, %f, %f (%x)\n", a, b, c, m_pushpc);

	// #### Wrong, need to check with the tgp_vr_cbox coordinates
	// Game only test sign, negative = collision
	fifoout_push_f(-1);
}

TGP_FUNCTION( model1::f49_swa )
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	float e = fifoin_pop_f();
	float f = fifoin_pop_f();
	(void)a;
	(void)b;
	(void)c;
	(void)d;
	(void)e;
	(void)f;
	M1_LOG(0, "TGP f49_swa %f, %f, %f, %f, %f, %f (%x)\n", a, b, c, d, e, f, m_pushpc);
}

TGP_FUNCTION( model1::f50_swa )
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	(void)a;
	(void)b;
	(void)c;
	(void)d;
	M1_LOG(0, "TGP f50_swa %f, %f, %f, %f (%x)\n", a, b, c, d, m_pushpc);
	fifoout_push_f(d);
}

TGP_FUNCTION( model1::f52 )
{
	M1_LOG(0, "TGP f52 (%x)\n", m_pushpc);
}

TGP_FUNCTION( model1::matrix_rdir )
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float norm = sqrtf(a*a+c*c);
	float t1, t2;
	(void)b;

	M1_LOG(0, "TGP matrix_rdir %f, %f, %f (%x)\n", a, b, c, m_pushpc);

	if(!norm) {
		c = 1;
		a = 0;
	} else {
		c /= norm;
		a /= norm;
	}

	t1 = m_cmat[6];
	t2 = m_cmat[0];
	m_cmat[6] = c*t1-a*t2;
	m_cmat[0] = a*t1+c*t2;
	t1 = m_cmat[7];
	t2 = m_cmat[1];
	m_cmat[7] = c*t1-a*t2;
	m_cmat[1] = a*t1+c*t2;
	t1 = m_cmat[8];
	t2 = m_cmat[2];
	m_cmat[8] = c*t1-a*t2;
	m_cmat[2] = a*t1+c*t2;
}

// A+(B-A)*t1 + (C-A)*t2 = P
static void tri_calc_pq(float ax, float ay, float bx, float by, float cx, float cy, float px, float py, float *t1, float *t2)
{
	float d;
	bx -= ax;
	cx -= ax;
	px -= ax;
	by -= ay;
	cy -= ay;
	py -= ay;
	d = bx*cy-by*cx;
	*t1 = (px*cy-py*cx)/d;
	*t2 = (bx*py-by*px)/d;
}

TGP_FUNCTION( model1::track_lookup )
{
	float a = fifoin_pop_f();
	u32 b = fifoin_pop();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	int offi, offd, len;
	float dist;
	int i;
	u32 entry;
	float height;

	M1_LOG(0, "TGP track_lookup %f, 0x%x, %f, %f (%x)\n", a, b, c, d, m_pushpc);

	offi = m_copro_data->as_u32(0x10+m_tgp_vr_select) + b;
	offd = m_copro_data->as_u32(0x20+m_tgp_vr_select);

	len = m_copro_data->as_u32(offi++);

	dist = -1;

//  behaviour = 0;
	height = 0.0;
	entry = 0;

	for(i=0; i<len; i++) {
		int j;
		int bpos = m_copro_data->as_u32(offi++);
		int posd = offd + bpos*0x10;
		const float *pts = (const float *)&m_copro_data->as_u32(posd);
		float ax = pts[12];
		float ay = pts[14];
		float az = pts[13];
		for(j=0; j<4; j++) {
			float t1, t2;
			int k = (j+1) & 3;
			tri_calc_pq(ax, ay, pts[3*j], pts[3*j+2], pts[3*k], pts[3*k+2], c, d, &t1, &t2);
			if(t1 >= 0 && t2 >= 0 && t1+t2 <= 1) {
				float z = az+t1*(pts[3*j+1]-az)+t2*(pts[3*k+1]-az);
				float d = (a-z)*(a-z);
				if(dist == -1 || d<dist) {
					dist = d;
//                  behaviour = m_copro_data->as_u32(posd+15);
					height = z;
					entry = bpos+i;
				}
			}
		}
	}

	m_copro_ram_data[0x0000] = 0; // non zero (ffffffff) = still searching
	m_copro_ram_data[0x0001] = f2u(height);
	m_copro_ram_data[0x0002] = entry;
}

TGP_FUNCTION( model1::f56 )
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	float e = fifoin_pop_f();
	float f = fifoin_pop_f();
	u32 g = fifoin_pop();
	(void)a;
	(void)b;
	(void)c;
	(void)d;
	(void)e;
	(void)f;
	(void)g;

	M1_LOG(0, "TGP f56 %f, %f, %f, %f, %f, %f, %d (%x)\n", a, b, c, d, e, f, g, m_pushpc);
	fifoout_push(0);
}

TGP_FUNCTION( model1::int_normal )
{
	M1_LOG(0, "TGP int_normal (%x)\n", m_pushpc);
	fifoout_push_f(u2f(m_copro_data->as_u32(m_tgp_int_adr+12)));
	fifoout_push_f(u2f(m_copro_data->as_u32(m_tgp_int_adr+13)));
	fifoout_push_f(u2f(m_copro_data->as_u32(m_tgp_int_adr+14)));
}

TGP_FUNCTION( model1::matrix_readt )
{
	M1_LOG(0, "TGP matrix_readt (%x)\n", m_pushpc);
	fifoout_push_f(m_cmat[9]);
	fifoout_push_f(m_cmat[10]);
	fifoout_push_f(m_cmat[11]);
}

TGP_FUNCTION( model1::acc_geti )
{
	M1_LOG(0, "TGP acc_geti (%x)\n", m_pushpc);
	fifoout_push((int)m_acc);
}

TGP_FUNCTION( model1::int_point )
{
	M1_LOG(0, "TGP int_point (%x)\n", m_pushpc);
	fifoout_push_f(m_tgp_int_px);
	fifoout_push_f(m_tgp_int_py);
	fifoout_push_f(m_tgp_int_pz);
}

TGP_FUNCTION( model1::col_setcirc )
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	M1_LOG(0, "TGP col_setcirc %f, %f, %f (%x)\n", a, b, c, m_pushpc);
	m_tgp_vr_circx = a;
	m_tgp_vr_circy = b;
	m_tgp_vr_circrad = c;
}

TGP_FUNCTION( model1::col_testpt )
{
	float x, y;
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	M1_LOG(0, "TGP col_testpt %f, %f (%x)\n", a, b, m_pushpc);
	x = a - m_tgp_vr_circx;
	y = b - m_tgp_vr_circy;
	fifoout_push_f(sqrtf(x*x+y*y) - m_tgp_vr_circrad);
}

TGP_FUNCTION( model1::push_and_ident )
{
	if(m_mat_stack_pos != MAT_STACK_SIZE) {
		memcpy(m_mat_stack[m_mat_stack_pos], m_cmat, sizeof(m_cmat));
		m_mat_stack_pos++;
	}
	M1_LOG(0, "TGP push_and_ident (depth=%d, pc=%x)\n", m_mat_stack_pos, m_pushpc);
	memset(m_cmat, 0, sizeof(m_cmat));
	m_cmat[0] = 1.0;
	m_cmat[4] = 1.0;
	m_cmat[8] = 1.0;
}

TGP_FUNCTION( model1::catmull_rom )
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	float e = fifoin_pop_f();
	float f = fifoin_pop_f();
	float g = fifoin_pop_f();
	float h = fifoin_pop_f();
	float i = fifoin_pop_f();
	float j = fifoin_pop_f();
	float k = fifoin_pop_f();
	float l = fifoin_pop_f();
	float m = fifoin_pop_f();
	float m2, m3;
	float w1, w2, w3, w4;

	M1_LOG(0, "TGP catmull_rom %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f (%x)\n", a, b, c, d, e, f, g, h, i, j, k, l, m, m_pushpc);

	m2 = m*m;
	m3 = m*m*m;

	w1 = 0.5f*(-m3+2*m2-m);
	w2 = 0.5f*(3*m3-5*m2+2);
	w3 = 0.5f*(-3*m3+4*m2+m);
	w4 = 0.5f*(m3-m2);

	fifoout_push_f(a*w1+d*w2+g*w3+j*w4);
	fifoout_push_f(b*w1+e*w2+h*w3+k*w4);
	fifoout_push_f(c*w1+f*w2+i*w3+l*w4);
}

TGP_FUNCTION( model1::distance )
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	M1_LOG(0, "TGP distance (%f, %f), (%f, %f) (%x)\n", a, b, c, d, m_pushpc);
	c -= a;
	d -= b;
	fifoout_push_f(sqrtf(c*c+d*d));
}

TGP_FUNCTION( model1::car_move )
{
	s16 a = fifoin_pop();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	float dx, dy;
	M1_LOG(0, "TGP car_move (%d, %f), (%f, %f) (%x)\n", a, b, c, d, m_pushpc);

	dx = b*tsin(a);
	dy = b*tcos(a);

	fifoout_push_f(dx);
	fifoout_push_f(dy);
	fifoout_push_f(c+dx);
	fifoout_push_f(d+dy);
}

TGP_FUNCTION( model1::cpa )
{
	float dv_x, dv_y, dv_z, dv2, dw_x, dw_y, dw_z, dt;

	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	float e = fifoin_pop_f();
	float f = fifoin_pop_f();
	float g = fifoin_pop_f();
	float h = fifoin_pop_f();
	float i = fifoin_pop_f();
	float j = fifoin_pop_f();
	float k = fifoin_pop_f();
	float l = fifoin_pop_f();
	M1_LOG(0, "TGP cpa %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f (%x)\n", a, b, c, d, e, f, g, h, i, j, k, l, m_pushpc);

	dv_x = (b-a) - (d-c);
	dv_y = (f-e) - (h-g);
	dv_z = (j-i) - (l-k);
	dv2 = dv_x*dv_x + dv_y*dv_y + dv_z*dv_z;
	if(dv2 < 0.001f)
		dt = 0;
	else {
		dw_x = a-c;
		dw_y = e-g;
		dw_z = i-k;
		dt = -(dw_x*dv_x + dw_y*dv_y + dw_z*dv_z)/dv2;
	}
	if(dt < 0)
		dt = 0;
	else if(dt > 1.0f)
		dt = 1.0f;

	dv_x = (a-c)*(1-dt) + (b-d)*dt;
	dv_y = (e-g)*(1-dt) + (f-h)*dt;
	dv_z = (i-k)*(1-dt) + (j-l)*dt;
	dv2 = dv_x*dv_x + dv_y*dv_y + dv_z*dv_z;

	fifoout_push_f(sqrtf(dv2));
}

TGP_FUNCTION( model1::vmat_store )
{
	u32 a = fifoin_pop();
	if(a<21)
		memcpy(m_mat_vector[a], m_cmat, sizeof(m_cmat));
	else
		M1_LOG(0, "TGP ERROR bad vector index\n");
	M1_LOG(0, "TGP vmat_store %d (%x)\n", a, m_pushpc);
}

TGP_FUNCTION( model1::vmat_restore )
{
	u32 a = fifoin_pop();
	if(a<21)
		memcpy(m_cmat, m_mat_vector[a], sizeof(m_cmat));
	else
		M1_LOG(0, "TGP ERROR bad vector index\n");
	M1_LOG(0, "TGP vmat_restore %d (%x)\n", a, m_pushpc);
}

TGP_FUNCTION( model1::vmat_mul )
{
	u32 a = fifoin_pop();
	u32 b = fifoin_pop();
	if(a<21 && b<21) {
		m_mat_vector[b][0]  = m_mat_vector[a][ 0]*m_cmat[0] + m_mat_vector[a][ 1]*m_cmat[3] + m_mat_vector[a][ 2]*m_cmat[6];
		m_mat_vector[b][1]  = m_mat_vector[a][ 0]*m_cmat[1] + m_mat_vector[a][ 1]*m_cmat[4] + m_mat_vector[a][ 2]*m_cmat[7];
		m_mat_vector[b][2]  = m_mat_vector[a][ 0]*m_cmat[2] + m_mat_vector[a][ 1]*m_cmat[5] + m_mat_vector[a][ 2]*m_cmat[8];
		m_mat_vector[b][3]  = m_mat_vector[a][ 3]*m_cmat[0] + m_mat_vector[a][ 4]*m_cmat[3] + m_mat_vector[a][ 5]*m_cmat[6];
		m_mat_vector[b][4]  = m_mat_vector[a][ 3]*m_cmat[1] + m_mat_vector[a][ 4]*m_cmat[4] + m_mat_vector[a][ 5]*m_cmat[7];
		m_mat_vector[b][5]  = m_mat_vector[a][ 3]*m_cmat[2] + m_mat_vector[a][ 4]*m_cmat[5] + m_mat_vector[a][ 5]*m_cmat[8];
		m_mat_vector[b][6]  = m_mat_vector[a][ 6]*m_cmat[0] + m_mat_vector[a][ 7]*m_cmat[3] + m_mat_vector[a][ 8]*m_cmat[6];
		m_mat_vector[b][7]  = m_mat_vector[a][ 6]*m_cmat[1] + m_mat_vector[a][ 7]*m_cmat[4] + m_mat_vector[a][ 8]*m_cmat[7];
		m_mat_vector[b][8]  = m_mat_vector[a][ 6]*m_cmat[2] + m_mat_vector[a][ 7]*m_cmat[5] + m_mat_vector[a][ 8]*m_cmat[8];
		m_mat_vector[b][9]  = m_mat_vector[a][ 9]*m_cmat[0] + m_mat_vector[a][10]*m_cmat[3] + m_mat_vector[a][11]*m_cmat[6] + m_cmat[9];
		m_mat_vector[b][10] = m_mat_vector[a][ 9]*m_cmat[1] + m_mat_vector[a][10]*m_cmat[4] + m_mat_vector[a][11]*m_cmat[7] + m_cmat[10];
		m_mat_vector[b][11] = m_mat_vector[a][ 9]*m_cmat[2] + m_mat_vector[a][10]*m_cmat[5] + m_mat_vector[a][11]*m_cmat[8] + m_cmat[11];
	} else
		M1_LOG(0, "TGP ERROR bad vector index\n");
	M1_LOG(0, "TGP vmat_mul %d, %d (%x)\n", a, b, m_pushpc);
}

TGP_FUNCTION( model1::vmat_read )
{
	u32 a = fifoin_pop();
	M1_LOG(0, "TGP vmat_read %d (%x)\n", a, m_pushpc);
	if(a<21) {
		int i;
		for(i=0; i<12; i++)
			fifoout_push_f(m_mat_vector[a][i]);
	} else {
		int i;
		M1_LOG(0, "TGP ERROR bad vector index\n");
		for(i=0; i<12; i++)
			fifoout_push_f(0);
	}
}

TGP_FUNCTION( model1::matrix_rtrans )
{
	M1_LOG(0, "TGP matrix_rtrans (%x)\n", m_pushpc);
	fifoout_push_f(m_cmat[ 9]);
	fifoout_push_f(m_cmat[10]);
	fifoout_push_f(m_cmat[11]);
}

TGP_FUNCTION( model1::matrix_unrot )
{
	M1_LOG(0, "TGP matrix_unrot (%x)\n", m_pushpc);
	memset(m_cmat, 0, 9*sizeof(m_cmat[0]));
	m_cmat[0] = 1.0;
	m_cmat[4] = 1.0;
	m_cmat[8] = 1.0;
}

TGP_FUNCTION( model1::f80 )
{
	M1_LOG(0, "TGP f80 (%x)\n", m_pushpc);
	//  m_cmat[9] = m_cmat[10] = m_cmat[11] = 0;
}

TGP_FUNCTION( model1::vmat_save )
{
	u32 a = fifoin_pop();
	int i;
	M1_LOG(0, "TGP vmat_save 0x%x (%x)\n", a, m_pushpc);
	for(i=0; i<16; i++)
		memcpy(m_copro_ram_data.get()+a+0x10*i, m_mat_vector[i], sizeof(m_cmat));
}

TGP_FUNCTION( model1::vmat_load )
{
	u32 a = fifoin_pop();
	int i;
	M1_LOG(0, "TGP vmat_load 0x%x (%x)\n", a, m_pushpc);
	for(i=0; i<16; i++)
		memcpy(m_mat_vector[i], m_copro_ram_data.get()+a+0x10*i, sizeof(m_cmat));
}

TGP_FUNCTION( model1::ram_setadr )
{
	m_copro_hle_ram_scan_adr = fifoin_pop() - 0x8000;
	M1_LOG(0, "TGP f0 ram_setadr 0x%x (%x)\n", m_copro_hle_ram_scan_adr+0x8000, m_pushpc);
}

TGP_FUNCTION( model1::groundbox_test )
{
	int out_x, out_y, out_z;
	float x, /*y,*/ z;
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();

	M1_LOG(0, "TGP groundbox_test %f, %f, %f (%x)\n", a, b, c, m_pushpc);
	x = m_cmat[0]*a+m_cmat[3]*b+m_cmat[6]*c+m_cmat[9];
	//y = m_cmat[1]*a+m_cmat[4]*b+m_cmat[7]*c+m_cmat[10];
	z = m_cmat[2]*a+m_cmat[5]*b+m_cmat[8]*c+m_cmat[11];

	out_x = x < m_tgp_vf_xmin || x > m_tgp_vf_xmax;
	out_z = z < m_tgp_vf_zmin || z > m_tgp_vf_zmax;
	out_y = 1; // Wrong, but untestable it seems.

	fifoout_push(out_x);
	fifoout_push(out_y);
	fifoout_push(out_z);
}

TGP_FUNCTION( model1::f89 )
{
	u32 a = fifoin_pop();
	u32 b = fifoin_pop();
	u32 c = fifoin_pop();
	u32 d = fifoin_pop();
	(void)a;
	(void)b;
	(void)c;
	M1_LOG(0, "TGP list set base 0x%x, 0x%x, %d, length=%d (%x)\n", a, b, c, d, m_pushpc);
	m_copro_hle_list_length = d;
}

TGP_FUNCTION( model1::f92 )
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	(void)a;
	(void)b;
	(void)c;
	(void)d;
	M1_LOG(0, "TGP f92 %f, %f, %f, %f (%x)\n", a, b, c, d, m_pushpc);
}

TGP_FUNCTION( model1::f93 )
{
	float a = fifoin_pop_f();
	(void)a;
	M1_LOG(0, "TGP f93 %f (%x)\n", a, m_pushpc);
}

TGP_FUNCTION( model1::f94 )
{
	u32 a = fifoin_pop();
	(void)a;
	M1_LOG(0, "TGP f94 %d (%x)\n", a, m_pushpc);
}

TGP_FUNCTION( model1::vmat_flatten )
{
	int i;
	float m[12];
	M1_LOG(0, "TGP vmat_flatten (%x)\n", m_pushpc);

	for(i=0; i<16; i++) {
		memcpy(m, m_mat_vector[i], sizeof(m_cmat));
		m[1] = m[4] = m[7] = m[10] = 0;

		m_mat_vector[i][0]  = m[ 0]*m_cmat[0] + m[ 1]*m_cmat[3] + m[ 2]*m_cmat[6];
		m_mat_vector[i][1]  = m[ 0]*m_cmat[1] + m[ 1]*m_cmat[4] + m[ 2]*m_cmat[7];
		m_mat_vector[i][2]  = m[ 0]*m_cmat[2] + m[ 1]*m_cmat[5] + m[ 2]*m_cmat[8];
		m_mat_vector[i][3]  = m[ 3]*m_cmat[0] + m[ 4]*m_cmat[3] + m[ 5]*m_cmat[6];
		m_mat_vector[i][4]  = m[ 3]*m_cmat[1] + m[ 4]*m_cmat[4] + m[ 5]*m_cmat[7];
		m_mat_vector[i][5]  = m[ 3]*m_cmat[2] + m[ 4]*m_cmat[5] + m[ 5]*m_cmat[8];
		m_mat_vector[i][6]  = m[ 6]*m_cmat[0] + m[ 7]*m_cmat[3] + m[ 8]*m_cmat[6];
		m_mat_vector[i][7]  = m[ 6]*m_cmat[1] + m[ 7]*m_cmat[4] + m[ 8]*m_cmat[7];
		m_mat_vector[i][8]  = m[ 6]*m_cmat[2] + m[ 7]*m_cmat[5] + m[ 8]*m_cmat[8];
		m_mat_vector[i][9]  = m[ 9]*m_cmat[0] + m[10]*m_cmat[3] + m[11]*m_cmat[6] + m_cmat[9];
		m_mat_vector[i][10] = m[ 9]*m_cmat[1] + m[10]*m_cmat[4] + m[11]*m_cmat[7] + m_cmat[10];
		m_mat_vector[i][11] = m[ 9]*m_cmat[2] + m[10]*m_cmat[5] + m[11]*m_cmat[8] + m_cmat[11];
	}
}

TGP_FUNCTION( model1::vmat_load1 )
{
	u32 a = fifoin_pop();
	M1_LOG(0, "TGP vmat_load1 0x%x (%x)\n", a, m_pushpc);
	memcpy(m_cmat, m_copro_ram_data.get()+a, sizeof(m_cmat));
}

TGP_FUNCTION( model1::ram_trans )
{
	float a = ram_get_f();
	float b = ram_get_f();
	float c = ram_get_f();
	M1_LOG(0, "TGP ram_trans (%x)\n", m_pushpc);
	m_cmat[ 9] += m_cmat[0]*a+m_cmat[3]*b+m_cmat[6]*c;
	m_cmat[10] += m_cmat[1]*a+m_cmat[4]*b+m_cmat[7]*c;
	m_cmat[11] += m_cmat[2]*a+m_cmat[5]*b+m_cmat[8]*c;
}

TGP_FUNCTION( model1::f98 )
{
	u32 a = fifoin_pop();
	(void)a;
	M1_LOG(0, "TGP load list start %d (%x)\n", a, m_pushpc);
	m_copro_hle_active_list_length = m_copro_hle_list_length;
	m_copro_hle_active_list_pos = 0;
}

TGP_FUNCTION( model1::f99 )
{
	M1_LOG(0, "TGP f99 (%x)\n", m_pushpc);
}

TGP_FUNCTION( model1::f100 )
{
	int i;
	M1_LOG(0, "TGP f100 get list (%x)\n", m_pushpc);
	for(i=0; i<m_copro_hle_list_length; i++)
		fifoout_push_f((m_rand() % 1000)/100.0);
}

TGP_FUNCTION( model1::groundbox_set )
{
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	float e = fifoin_pop_f();
	float f = fifoin_pop_f();
	float g = fifoin_pop_f();
	M1_LOG(0, "TGP groundbox_set %f, %f, %f, %f, %f, %f, %f (%x)\n", a, b, c, d, e, f, g, m_pushpc);
	m_tgp_vf_xmin = e;
	m_tgp_vf_xmax = d;
	m_tgp_vf_zmin = g;
	m_tgp_vf_zmax = f;
	m_tgp_vf_ygnd = b;
	m_tgp_vf_yflr = a;
	m_tgp_vf_yjmp = c;
}

TGP_FUNCTION( model1::f102 )
{
	float px, py, pz;
	float a = fifoin_pop_f();
	float b = fifoin_pop_f();
	float c = fifoin_pop_f();
	float d = fifoin_pop_f();
	float e = fifoin_pop_f();
	u32 f = fifoin_pop();
	u32 g = fifoin_pop();
	u32 h = fifoin_pop();

	m_ccount++;

	M1_LOG(0, "TGP f0 mve_calc %f, %f, %f, %f, %f, %d, %d, %d (%d) (%x)\n", a, b, c, d, e, f, g, h, m_ccount, m_pushpc);

	px = u2f(m_copro_ram_data[m_copro_hle_ram_scan_adr+0x16]);
	py = u2f(m_copro_ram_data[m_copro_hle_ram_scan_adr+0x17]);
	pz = u2f(m_copro_ram_data[m_copro_hle_ram_scan_adr+0x18]);

	//  memset(m_cmat, 0, sizeof(m_cmat));
	//  m_cmat[0] = 1.0;
	//  m_cmat[4] = 1.0;
	//  m_cmat[8] = 1.0;

	px = c;
	py = d;
	pz = e;

#if 1
	m_cmat[ 9] += m_cmat[0]*a+m_cmat[3]*b+m_cmat[6]*c;
	m_cmat[10] += m_cmat[1]*a+m_cmat[4]*b+m_cmat[7]*c;
	m_cmat[11] += m_cmat[2]*a+m_cmat[5]*b+m_cmat[8]*c;
#else
	m_cmat[ 9] += px;
	m_cmat[10] += py;
	m_cmat[11] += pz;
#endif

	M1_LOG(0, "    f0 mve_calc %f, %f, %f\n", px, py, pz);

	fifoout_push_f(c);
	fifoout_push_f(d);
	fifoout_push_f(e);
	fifoout_push(f);
	fifoout_push(g);
	fifoout_push(h);
}

TGP_FUNCTION( model1::f103 )
{
	m_copro_hle_ram_scan_adr = fifoin_pop() - 0x8000;
	M1_LOG(0, "TGP f0 mve_setadr 0x%x (%x)\n", m_copro_hle_ram_scan_adr, m_pushpc);
	ram_get_i();
}

// Addresses in daytona's and VR's TGP programs
const struct model1::function model1::ftab_vf[] = {
	{ &model1::fadd,            2 }, /* 0x00 */ // 0b5 09b
	{ &model1::fsub,            2 },            // 0ba 0a0
	{ &model1::fmul,            2 },            // 0bf 0a5
	{ &model1::fdiv,            2 },            // 0c4 0aa
	{ nullptr,                        0 },            // 0d2 0b9
	{ &model1::matrix_push,     0 },            // 0e3 0cc
	{ &model1::matrix_pop,      0 },            // 0f7 0e0
	{ &model1::matrix_write,   12 },            // 106 0ef
	{ &model1::clear_stack,     0 },            // 10a 0f4
	{ &model1::matrix_mul,     12 },            // 10e 0f8
	{ &model1::anglev,          2 },            // 154 13f
	{ nullptr,                        0 },            // 15d 149
	{ nullptr,                        0 },            // 19f 18a
	{ &model1::track_select,    1 },            // 1b8 1a5
	{ &model1::load_base,       4 },            // 1bb 1a8
	{ &model1::anglep,          4 },            // 1c1 1ae

	{ &model1::matrix_ident,    0 }, /* 0x10 */ // 1d2 1c1
	{ &model1::matrix_read,     0 },            // 1d9 1c8
	{ &model1::matrix_trans,    3 },            // 1dd 1d1
	{ &model1::matrix_scale,    3 },            // 1f3 1e8
	{ &model1::matrix_rotx,     1 },            // 20a 200
	{ &model1::matrix_roty,     1 },            // 223 21a
	{ &model1::matrix_rotz,     1 },            // 23b 233
	{ nullptr,                        0 },            // 253 24c
	{ &model1::track_read_quad, 1 },            // 2aa 2a8
	{ nullptr,                        0 },            // 2b0 2b0
	{ &model1::transform_point, 3 },            // 2b2 2b4
	{ &model1::fsin_m1,         1 },            // 2cb 2d7
	{ &model1::fcos_m1,         1 },            // 2ce 2df
	{ &model1::fsinm_m1,        2 },            // 2d1 2e7
	{ &model1::fcosm_m1,        2 },            // 2d7 2f1
	{ &model1::distance3,       6 },            // 2dd 2fb

	{ nullptr,                        0 }, /* 0x20 */ // 2ff 31e
	{ nullptr,                        0 },            // 2ff 31e
	{ nullptr,                        0 },            // 300 31f
	{ nullptr,                        0 },            // 304 323
	{ &model1::acc_set,         1 },            // 308 327
	{ &model1::acc_get,         0 },            // 30a 32a
	{ &model1::acc_add,         1 },            // 30c 32e
	{ &model1::acc_sub,         1 },            // 311 333
	{ &model1::acc_mul,         1 },            // 316 338
	{ &model1::acc_div,         1 },            // 31b 33d
	{ &model1::f42,             3 },            // 329 34b
	{ &model1::f43,             6 },            // 36c 38f
	{ &model1::track_read_tri,  1 },            // 3c2 3e2
	{ &model1::fsqrt,           1 },            // 3c7 3e9
	{ &model1::vlength,         3 },            // 3cf 3f2
	{ nullptr,                        0 },            // 3ef 411

	{ &model1::track_read_info, 1 }, /* 0x30 */ // 410 431
	{ &model1::colbox_set,     12 },            // 413 436
	{ &model1::colbox_test,     3 },            // 417 43b
	{ nullptr,                        0 },            // 43b 463
	{ nullptr,                        0 },            // 44d 478
	{ nullptr,                        0 },            // 452 47d
	{ &model1::track_lookup,    4 },            // 457 482
	{ nullptr,                        0 },            // 51a 522
	{ nullptr,                        0 },            // 521 527
	{ nullptr,                        0 },            // 52f 535
	{ nullptr,                        0 },            // 53d 543
	{ nullptr,                        0 },            // 545 54c
	{ nullptr,                        0 },            // 558 55e
	{ nullptr,                        0 },            // 559 55f
	{ nullptr,                        0 },            // 5c6 5cd
	{ nullptr,                        0 },            // 5e9 5f4

	{ &model1::col_setcirc,     3 }, /* 0x40 */ // 5f3 5fe
	{ &model1::col_testpt,      2 },            // 5fa 603
	{ nullptr,                        0 },            // 615 61e
	{ &model1::distance,        4 },            // 631 639
	{ nullptr,                        0 },            // 63f 648
	{ nullptr,                        0 },            // 643 64e
	{ nullptr,                        0 },            // 64b 657
	{ &model1::car_move,        4 },            // 661 66e
	{ &model1::cpa,            12 },            // 7d9
	{ nullptr,                        0 },
	{ &model1::vmat_store,      1 },
	{ &model1::vmat_restore,    1 },
	{ nullptr,                        0 },
	{ &model1::vmat_mul,        2 },
	{ &model1::vmat_read,       1 },
	{ &model1::matrix_unrot,    0 },

	{ &model1::f80,             0 }, /* 0x50 */
	{ nullptr,                        0 },
	{ &model1::matrix_rtrans,   0 },
	{ nullptr,                        0 },
	{ &model1::vmat_save,       1 },
	{ &model1::vmat_load,       1 },
	{ &model1::ram_setadr,      1 },
	{ &model1::groundbox_test,  3 },
	{ nullptr,                        0 },
	{ &model1::f89,             4 },
	{ nullptr,                        0 },
	{ nullptr,                        0 },
	{ &model1::f92,             4 },
	{ &model1::f93,             1 },
	{ &model1::f94,             1 },
	{ &model1::vmat_flatten,    0 },

	{ &model1::vmat_load1,      1 }, /* 0x60 */
	{ &model1::ram_trans,       0 },
	{ &model1::f98,             1 },
	{ &model1::f99,             0 },
	{ &model1::f100,            0 },
	{ &model1::groundbox_set,   7 },
	{ &model1::f102,            8 },
	{ &model1::f103,            1 }
};

// Used in swa scene 1 and unemulated:
//   f49_swa

void model1::copro_hle_vf()
{
	if(m_copro_hle_active_list_length) {
		while(m_copro_hle_active_list_pos < m_copro_hle_active_list_length && !m_copro_fifo_in->is_empty()) {
			u32 v = m_copro_fifo_in->pop();
			m_copro_hle_active_list_pos++;
			M1_LOG(0, "TGP load list %d/%d: %08x\n", m_copro_hle_active_list_pos, m_copro_hle_active_list_length, v);
		}
		if(m_copro_hle_active_list_pos == m_copro_hle_active_list_length)
			m_copro_hle_active_list_pos = m_copro_hle_active_list_length = 0;
		if(m_copro_fifo_in->is_empty())
			return;
	}

 next:
	u32 f = m_copro_fifo_in->peek(0) >> 23; // Extracts the exponent of the ieee float

	//  M1_LOG(0, "function %02x size %d/%d\n", f, int(m_copro_fifo_in->size() - 1), ftab_vf[f].count);

	if(f < std::size(ftab_vf) && ftab_vf[f].cb != nullptr) {
		unsigned np = ftab_vf[f].count;
		if(m_copro_fifo_in->size() >= np+1) {
			m_copro_fifo_in->pop();
			m_tgp_calls[f]++;
			(this->*ftab_vf[f].cb)();
			if(!m_copro_fifo_in->is_empty())
				goto next;
		}

	} else {
		M1_LOG(0, "TGP function %d unimplemented (%x)\n", f, m_pushpc);
		m_tgp_missing[f]++;
		m_copro_fifo_in->pop();
	}
}

u16 model1::v60_copro_ram_adr_r(offs_t offset, u16 mem_mask)
{
	return m_v60_copro_ram_adr;
}

void model1::v60_copro_ram_adr_w(offs_t offset, u16 data, u16 mem_mask)
{
	COMBINE_DATA(&m_v60_copro_ram_adr);
}

u16 model1::v60_copro_ram_r(offs_t offset, u16 mem_mask)
{
	u16 r;

	if (!offset)
		r = m_copro_ram_data[m_v60_copro_ram_adr & 0x7fff];

	else {
		r = m_copro_ram_data[m_v60_copro_ram_adr & 0x7fff] >> 16;

		if(m_v60_copro_ram_adr & 0x8000)
			m_v60_copro_ram_adr ++;
	}

	return r;
}

void model1::v60_copro_ram_w(offs_t offset, u16 data, u16 mem_mask)
{
	COMBINE_DATA(m_v60_copro_ram_latch + offset);

	if (offset) {
		u32 v = m_v60_copro_ram_latch[0] | (m_v60_copro_ram_latch[1] << 16);
		m_copro_ram_data[m_v60_copro_ram_adr & 0x7fff] = v;
		if(m_v60_copro_ram_adr & 0x8000)
			m_v60_copro_ram_adr++;
	}
}

u16 model1::v60_copro_fifo_r(offs_t offset, u16 mem_mask)
{
	if (!offset) {
		if (m_copro_fifo_out->is_empty())
			m_tgp_underflow[m_maincpu.pc()]++;
		m_v60_copro_fifo_r = m_copro_fifo_out->pop();
		return m_v60_copro_fifo_r;

	} else
		return m_v60_copro_fifo_r >> 16;
}

void model1::v60_copro_fifo_w(offs_t offset, u16 data, u16 mem_mask)
{
	if(offset) {
		m_v60_copro_fifo_w = (m_v60_copro_fifo_w & 0x0000ffff) | (data << 16);
		m_pushpc = m_maincpu.pc();
		m_copro_fifo_in->push(u32(m_v60_copro_fifo_w));

	} else
		m_v60_copro_fifo_w = (m_v60_copro_fifo_w & 0xffff0000) | data;
}

void model1::copro_ramadr_w(offs_t offset, u32 data, u32 mem_mask)
{
	COMBINE_DATA(&m_copro_ram_adr);
}

u32 model1::copro_ramadr_r(offs_t offset, u32 mem_mask)
{
	return m_copro_ram_adr;
}

void model1::copro_ramdata_w(offs_t offset, u32 data, u32 mem_mask)
{
	COMBINE_DATA(&m_copro_ram_data[m_copro_ram_adr & 0x7fff]);
	m_copro_ram_adr ++;
}

u32 model1::copro_ramdata_r(offs_t offset, u32 mem_mask)
{
	u32 val = m_copro_ram_data[m_copro_ram_adr & 0x7fff];
	m_copro_ram_adr ++;
	return val;
}



void model1::copro_sincos_w(offs_t offset, u32 data, u32 mem_mask)
{
	COMBINE_DATA(&m_copro_sincos_base);
}

u32 model1::copro_sincos_r(offs_t offset, u32 mem_mask)
{
	offs_t ang = m_copro_sincos_base + offset * 0x4000;
	offs_t index = ang & 0x3fff;
	if(ang & 0x4000)
		index ^= 0x3fff;
	u32 result = m_copro_tables[index];
	if(ang & 0x8000)
		result ^= 0x80000000;
	return result;
}

void model1::copro_inv_w(offs_t offset, u32 data, u32 mem_mask)
{
	COMBINE_DATA(&m_copro_inv_base);
}

u32 model1::copro_inv_r(offs_t offset, u32 mem_mask)
{
	offs_t index = ((m_copro_inv_base >> 9) & 0x3ffe) | (offset & 1);
	u32 result = m_copro_tables[index | 0x8000];
	u8 bexp = (m_copro_inv_base >> 23) & 0xff;
	u8 exp = (result >> 23) + (0x7f - bexp);
	result = (result & 0x807fffff) | (exp << 23);
	if(m_copro_inv_base & 0x80000000)
		result ^= 0x80000000;
	return result;
}

void model1::copro_isqrt_w(offs_t offset, u32 data, u32 mem_mask)
{
	COMBINE_DATA(&m_copro_isqrt_base);
}

u32 model1::copro_isqrt_r(offs_t offset, u32 mem_mask)
{
	offs_t index = 0x2000 ^ (((m_copro_isqrt_base>> 10) & 0x3ffe) | (offset & 1));
	u32 result = m_copro_tables[index | 0xc000];
	u8 bexp = (m_copro_isqrt_base >> 24) & 0x7f;
	u8 exp = (result >> 23) + (0x3f - bexp);
	result = (result & 0x807fffff) | (exp << 23);
	if(!(offset & 1))
		result &= 0x7fffffff;
	return result;
}

void model1::copro_atan_w(offs_t offset, u32 data, u32 mem_mask)
{
	COMBINE_DATA(&m_copro_atan_base[offset]);
}

u32 model1::copro_atan_r(offs_t offset, u32 mem_mask)
{
	offs_t index = (m_copro_atan_base[3] << 1);
	if(index == 0x4000)
		index = 0x3fff;
	u32 result = m_copro_tables[index | 0x4000];

	bool s0 = m_copro_atan_base[0] & 0x80000000;
	bool s1 = m_copro_atan_base[1] & 0x80000000;
	bool s2 = m_copro_atan_base[2] & 0x80000000;

	if(s0 ^ s1 ^ s2)
		result >>= 16;
	if(s2)
		result += 0x4000;
	if((s0 && !s2) || (s1 && s2))
		result += 0x8000;

	return result & 0xffff;
}

void model1::copro_data_w(offs_t offset, u32 data, u32 mem_mask)
{
	COMBINE_DATA(&m_copro_data_base);
}

u32 model1::copro_data_r(offs_t offset, u32 mem_mask)
{
	offs_t index = (m_copro_data_base & ~0x7fff) | offset;
	index &= (m_copro_data->bytes() >> 2) - 1;
	return m_copro_data->as_u32(index);
}
