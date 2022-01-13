/* Your test program. */
long g_a[4], g_b[5][3];

struct A {
    long x, y;
    long q[5][4];
    struct B {
        long q, r, s;
    } z;
} a, b[3];

void main() {
    long m1[4][3], m2[3][4];
    long m3[3][3], m4[4][4];
    long i, j, k;
    long l_a[3], l_b[7][4], l_c[7][8][9];

    j = 8 + i;
    m3[i][m4[j][i]] = k;
    m3[i][j] = m1[k][j] + m2[i][k];

    b[2].z.q = 6;

    a.z.r = 987654321;
    a.y = 1;
    a.x = 2;
    l_c[1][2][3] = 4;
    b[1].q[2][3] = 4;

    b[0].x = 7;
    b[0].y = 8;
    b[a.y].x = b[a.x].y;
    b[b[a.x].q[2][3]].z.s = 0;


    g_b[4][1] = 99;
    g_b[1][2] = 3;
    g_a[1] = 7;
    g_a[g_b[1][2]] = 4;

    l_b[4][1] = 99;
    l_b[6][3] = 0;
    l_a[1] = 7;
    l_a[l_b[1][2]] = 4;
}
