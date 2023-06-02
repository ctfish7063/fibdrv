import numpy as np
import subprocess
import argparse
parser = argparse.ArgumentParser()
parser.add_argument("-n", "--naive", help="switch to naive mode", action="store_true", default=False)
parser.add_argument("-r", "--runs", help="set number of runs")
parser.add_argument("-f", "--fib", help="set the most number of fibonacci")

def outlier_filter(datas, threshold = 2):
    datas = np.array(datas)
    if datas.std() == 0:
        return datas
    z = np.abs((datas - datas.mean()) / datas.std())
    return datas[z < threshold]

def data_processing(data_set, n):
    catgories = data_set[0].shape[0]
    samples = data_set[0].shape[1]
    final = np.zeros((catgories, samples))
    if np.isnan(data_set).any():
        print("Warning: NaN detected in data set")
    for c in range(catgories):        
        for s in range(samples):
            final[c][s] =                                                    \
                outlier_filter([data_set[i][c][s] for i in range(n)]).mean()
    return final

if __name__ == "__main__":
    args = parser.parse_args()
    if args.naive:
        flag = "naive"
    else:
        flag = "fast"
    if args.runs and int(args.runs) > 0:
        runs = int(args.runs)
    else:
        runs = 50
    if args.fib and int(args.fib) > 0:
        fib = int(args.fib)
    else:
        fib = 10000
    cmd  = f"sudo cset proc -e isolated -- sh -c './test {flag} {fib} > data.txt' > /dev/null"
    print(f"Run {runs} calculating upto fib[{fib}] in {flag} mode")
    Ys = []
    for i in range(runs):
        comp_proc = subprocess.run(cmd, shell = True)
        output = np.loadtxt('data.txt', dtype = 'float').T
        Ys.append(np.delete(output,0,0))
    X = output[0]
    Y = data_processing(Ys, runs)
    np.savetxt(f'{flag}.txt', np.column_stack((X,Y.T)), fmt = '%i', delimiter = ' ')