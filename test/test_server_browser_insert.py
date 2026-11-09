import os, re, subprocess, tempfile, unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SOURCE = os.path.join(ROOT, 'src', 'PC', 'ui_mp', 'ui_main_mp.c')

# UI_BinaryInsertServer as ui_main_mp.c has it, over a list of ints that sort
# by value; it prints the list after inserting each argument
PROGRAM = r'''
#include <stdio.h>
#include <stdlib.h>
static struct { struct { int numDisplayServers, displayServers[64], sortKey, sortDir; } serverStatus; } sharedUiInfo;
static struct { struct { int integer; } current; } netSource, *ui_netSource = &netSource;
static int keys[64];
static int LAN_CompareServers(int source, int sortKey, int sortDir, int a, int b)
{
    return (keys[a] > keys[b]) - (keys[a] < keys[b]);
}
static void UI_InsertServerAtPosition(int serverIndex, int position)
{
    int *list = sharedUiInfo.serverStatus.displayServers;
    int j;

    if (position < 0 || sharedUiInfo.serverStatus.numDisplayServers < position)
        return;
    for (j = sharedUiInfo.serverStatus.numDisplayServers++; j > position; j--)
        list[j] = list[j - 1];
    list[position] = serverIndex;
}
%s
int main(int argc, char **argv)
{
    int i;
    for (i = 1; i < argc; i++) {
        keys[i] = atoi(argv[i]);
        UI_BinaryInsertServer(i);
    }
    for (i = 0; i < sharedUiInfo.serverStatus.numDisplayServers; i++)
        printf("%%d ", keys[sharedUiInfo.serverStatus.displayServers[i]]);
    return 0;
}
'''


class ServerBrowserInsert(unittest.TestCase):
    """The server browser lists every server it is given, in order: the
    reconstructed search listed none, putting the first at 1 of an empty list."""

    def run_insert(self, keys):
        with open(SOURCE) as f:
            functions = re.findall(r'static void UI_BinaryInsertServer\(int serverIndex\)\n\{.*?\n\}\n', f.read(), re.S)
        self.assertEqual(len(functions), 2)
        self.assertEqual(functions[0], functions[1])
        with tempfile.TemporaryDirectory() as tmp:
            source = os.path.join(tmp, 'p.c')
            binary = os.path.join(tmp, 'p')
            with open(source, 'w') as f:
                f.write(PROGRAM % functions[0])
            subprocess.run(['gcc', source, '-o', binary], check=True)
            out = subprocess.run([binary] + [str(k) for k in keys], capture_output=True, text=True, check=True, timeout=10).stdout
        return [int(k) for k in out.split()]

    def test_orders(self):
        for keys in ([5], [3, 1, 2], [1, 2, 3, 4, 5, 6], [6, 5, 4, 3, 2, 1], [4, 9, 1, 7, 3, 8, 2, 6, 5, 0]):
            self.assertEqual(self.run_insert(keys), sorted(keys), keys)

    def test_equal_keys(self):
        self.assertEqual(self.run_insert([2, 2, 1, 2, 3]), [1, 2, 2, 2, 3])


if __name__ == '__main__':
    unittest.main()
