import sys, json
import random
import csv
import string
'''
You should implement this script to generate test data for your
merge sort program.

The schema definition should be separate from the data generation
code. See example schema file `schema_example.json`.
'''



def generate_data(schema, out_file, nrecords):
  '''
  Generate data according to the `schema` given,
  and write to `out_file`.
  `schema` is an list of dictionaries in the form of:
    [ 
      {
        'name' : <attribute_name>, 
        'length' : <fixed_length>,
        ...
      },
      ...
    ]
  `out_file` is the name of the output file.
  The output file must be in csv format, with a new line
  character at the end of every record.
  '''

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
  print ("Generating %d records" % nrecords)

if __name__ == '__main__':

  if not len(sys.argv) == 2:
    print ("data_generator.py <# of records>")
    sys.exit(0)

  nrecords = int(sys.argv[1])
  schema = json.load(open("schema.json"))
  output = "data.csv"
  
  generate_data(schema, output, nrecords)

