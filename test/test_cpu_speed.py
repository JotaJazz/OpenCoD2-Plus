import os, subprocess, tempfile, unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

PROGRAM = r'''
#include <stdio.h>
float MacFeatures_CPUMHzFrom(const char *maxFreqPath, const char *cpuinfoPath);
int main(int argc, char **argv)
{
    printf("%.1f ", MacFeatures_CPUMHzFrom(argv[1], argv[2]));   /* the rated maximum */
    printf("%.1f ", MacFeatures_CPUMHzFrom("/nonexistent", argv[2]));   /* else cpuinfo's */
    printf("%.1f", MacFeatures_CPUMHzFrom("/nonexistent", "/nonexistent"));   /* else 800 */
    return 0;
}
'''


class CpuSpeed(unittest.TestCase):
    """The Linux client's CPU speed is the rated maximum (cpufreq's
    cpuinfo_max_freq), steady from run to run as Windows' ~MHz: cpuinfo's
    "cpu MHz" is the current clock, which moved more than the 10% that
    Sys_HasInfoChanged allows and reset the profile's settings every start."""

    def test_cpu_mhz(self):
        with tempfile.TemporaryDirectory() as tmp:
            src = os.path.join(tmp, 't.c')
            exe = os.path.join(tmp, 't')
            maxfreq = os.path.join(tmp, 'max')
            cpuinfo = os.path.join(tmp, 'cpuinfo')
            with open(src, 'w') as f:
                f.write(PROGRAM)
            with open(maxfreq, 'w') as f:
                f.write('5700000\n')
            with open(cpuinfo, 'w') as f:
                f.write('processor\t: 0\nmodel name\t: x\ncpu MHz\t\t: 1234.500\n')
            subprocess.run(['gcc', '-m32', '-I', os.path.join(ROOT, 'src'), '-I', os.path.join(ROOT, 'src', 'headers'), src,
                            os.path.join(ROOT, 'src', 'Mac', 'Tools', 'MacFeatures.c'), '-o', exe], check=True)
            out = subprocess.run([exe, maxfreq, cpuinfo], capture_output=True, text=True, check=True).stdout
        self.assertEqual(out, '5700.0 1234.5 800.0')


if __name__ == '__main__':
    unittest.main()
