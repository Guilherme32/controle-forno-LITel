import requests
import os
import filecmp
import gzip


ip_addr = "http://192.168.0.101/"
web_path = "../web_code/"

file_ip = ip_addr + "d3.v4.min.js"
r = requests.get(file_ip, stream=True)

with open("temp", "wb") as temp_file:
    temp_file.write(r.content)


# print(r.content)
with gzip.open("../web_code/d3.v4.min.js.gz") as gfile:
    print(gfile.read() == r.content)

print(filecmp.cmp("../web_code/d3.v4.min.js.gz", "temp", shallow=False))

# for filename in os.listdir("../web_code/"):
#     file_ip = ip_addr + filename
#     r = requests.get(file_ip)
#     print(r.text)
