import matplotlib.pyplot as plt
import os
import time
import sys, json
import random
import csv
import string


def generate_data(schema, out_file, nrecords):
    # records = []
    with open(out_file, mode="w", newline="") as file:
        writer = csv.writer(file)
        for i in range(nrecords):
            record = []
            for attr in schema:
                _name = attr['name']
                _length = attr['length']
                _type = attr['type']
                if _type != 'string':
                    _distribution = attr['distribution']
                    if _distribution['name'] == "uniform":
                        _min = _distribution['min']
                        _max = _distribution['max']
                        record.append(str(random.uniform(_min, _max))[:_length])
                    elif _distribution['name'] == "normal":
                        _mu = _distribution['mu']
                        _sigma = _distribution['sigma']
                        _min = _distribution['min']
                        _max = _distribution['max']
                        record.append(str(min(max(random.normalvariate(_mu, _sigma), _min), _max))[:_length])
                else:
                    random_string = ''.join(random.choice(string.ascii_letters) for i in range(_length))
                    record.append(random_string)
            writer.writerow(record)
    # print ("Generating %d records" % nrecords)


def generate_diff_size_data(input_file):
    byte_offset = 30
    num_records = 150000
    start_time = time.time()
    data_size = []
    for i in range(10):
        output_file = f"data{i}.csv"
        byte_offset_total = (i+1) * byte_offset * num_records
        with open(input_file, "rb") as infile, open(output_file, "wb") as outfile:
            outfile.write(infile.read(byte_offset_total))
        data_size.append(byte_offset_total)

    end_time = time.time()
    exec_time = end_time - start_time
    print(f"Execution time: {exec_time:.2f} seconds")
    return data_size


def msort_diff_file_size(data_size):
    msort_time = []
    mem_capacity = 40000
    k = 4
    for i in range(10):
        msort_cmd = f"./msort schema.json data{i}.csv out.csv {mem_capacity} {k} cgpa start_year"
        s = os.popen(msort_cmd)
        msort_time.append(float(s.readline()))
        s.close()
        print(f"msort_diff_file_size: data{i}.csv")

    plt.plot(data_size, msort_time)
    plt.ylabel("msort_time")
    plt.xlabel("file_size")
    plt.title(f"Fix the parameters mem_capacity({mem_capacity * 30}) and k({k})")
    plt.savefig('assets/4.jpg')
    plt.close()


def msort_diff_mem_capacity():
    mem_capacity = []
    msort_time = []
    base_mem = 50000
    for i in range(10):
        mem_capacity.append(base_mem * (i + 1) * 30)
        msort_cmd = f"./msort schema.json data9.csv out.csv {base_mem * (i + 1)} 4 cgpa start_year"
        s = os.popen(msort_cmd)
        msort_time.append(float(s.readline()))
        s.close()
        print(f"msort_diff_mem_capacity: {base_mem * (i + 1)}")
    plt.plot(mem_capacity, msort_time)
    plt.ylabel("msort_time")
    plt.xlabel("mem_capacity")
    plt.title(f"Fix the parameter k(4) and the file size(150MB)")
    plt.savefig('assets/5.jpg')
    plt.close()


def msort_diff_k():
    msort_time = []
    k = 2
    k_ = []
    for i in range(10):
        k_.append(k)
        msort_cmd = f"./msort schema.json data9.csv out.csv 50000 {k} cgpa start_year"
        s = os.popen(msort_cmd)
        msort_time.append(float(s.readline()))
        s.close()
        print(f"msort_diff_k: {k}")
        k = k + 1

    plt.plot(k_, msort_time)
    plt.ylabel("msort_time")
    plt.xlabel("k")
    plt.title(f"Fix the parameter mem_capacity(150000) and the file_size(150MB)")
    plt.savefig('assets/6.jpg')
    plt.close()


def compare_bsort_msort_diff_file_size(data_size):
    good_msort_time = []
    good_msort_mem_capacity = 40000
    good_k = 4

    bad_msort_time = []
    bad_msort_mem_capacity = 333
    bad_k = 50

    bsort_time = []

    for i in range(10):
        good_msort_cmd = f"./msort schema.json data{i}.csv out.csv {good_msort_mem_capacity} {good_k} cgpa start_year"
        s = os.popen(good_msort_cmd)
        good_msort_time.append(float(s.readline()))
        s.close()

        bad_msort_cmd = f"./msort schema.json data{i}.csv out.csv {bad_msort_mem_capacity} {bad_k} cgpa start_year"
        s = os.popen(bad_msort_cmd)
        bad_msort_time.append(float(s.readline()))
        s.close()

        bsort_cmd = f"./bsort schema.json data{i}.csv out.csv cgpa start_year"
        s = os.popen(bsort_cmd)
        bsort_time.append(float(s.readline()))
        s.close()
        d = os.popen("rm -rf ./leveldb_dir")
        d.close()

        print(i)

    plt.plot(data_size, good_msort_time, 's-', color='r', label="msort MEM = 4000, k = 4")
    plt.plot(data_size, bad_msort_time, 'o-', color='g', label="msort MEM = 333, k = 50")
    plt.plot(data_size, bsort_time, 'v-', color='y', label="bsort")
    plt.legend(loc = "best")
    plt.ylabel("time")
    plt.xlabel("file_size")
    plt.title("Compare the performance of msort against bsort")
    plt.savefig('assets/7.jpg')
    plt.close()


if __name__ == '__main__':
    num_of_records = 10000000
    # generate_data(json.load(open("schema.json")), "data.csv", num_of_records)
    data_size = generate_diff_size_data("data.csv")
    # # msort_diff_file_size(data_size)
    # msort_diff_mem_capacity()
    # msort_diff_k()
    compare_bsort_msort_diff_file_size(data_size)