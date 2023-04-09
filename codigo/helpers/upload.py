import requests
import os
import gzip


upload_ip_addr = "http://192.168.0.101/upload_file"
get_ip_addr = "http://192.168.0.101/"
web_path = "../web_code/"


def upload_file(filename):
    print(f"Sending {filename}", end="")

    with open(os.path.join(web_path, filename), "rb") as file:
        upload_ip = upload_ip_addr + "?filename=" + filename
        r = requests.post(upload_ip, data=file)
        print(f" -> {r.text}")


def check_file(filename):
    print(f"Checking {filename}", end="")

    web_filename = filename[:-3] if filename.endswith(".gz") else filename
    get_ip = get_ip_addr + web_filename
    r = requests.get(get_ip)

    ok = False
    og_file = os.path.join(web_path, filename)

    if filename.endswith(".gz"):
        with gzip.open(og_file) as file:
            ok = file.read() == r.content
    else:
        with open(og_file, "rb") as file:
            ok = file.read() == r.content

    print(" -> {}".format("matches" if ok else "Doesn't match"))

    return ok


def main():
    for filename in os.listdir("../web_code/"):
        file_ok = False
        attempts = 0

        while (not file_ok and attempts < 10):
            upload_file(filename)
            file_ok = check_file(filename)
            attempts += 1

    all_ok = True
    for filename in os.listdir(web_path):
        if not check_file(filename):
            all_ok = False

    if all_ok:
        print("All files sent successfully")
    else:
        print("At least one file was not sent correctly")


if __name__ == "__main__":
    main()
