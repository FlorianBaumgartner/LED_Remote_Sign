from multiprocessing import Pool, cpu_count
from functools import partial
from bs4 import BeautifulSoup
import requests
import sys
import re
import os


def download(url, urls):
    print("test", flush=True)
    sys.stdout.flush()
    count = urls.index(url)
    response = requests.get('https://emojipedia.org' + url)
    html_soup = BeautifulSoup(response.text, 'html.parser')
    emoji = html_soup.find_all('div', class_=
                               'vendor-container vendor-rollout-target')[0]
    link = re.findall(r'srcset="+[\w\.:/-]+', str(emoji))[0][8:]
    name = link.split("285/")[1][:-4].replace("-", "_")
    r = requests.get(link)
    open(f'raw/E{count:04d}_{name}.png', 'wb').write(r.content)
    print(f"{count}/{len(urls)}: {name}", flush=True)
    sys.stdout.flush()
    return True


if __name__ == "__main__":
    response = requests.get('https://emojipedia.org/apple/')
    html_soup = BeautifulSoup(response.text, 'html.parser')
    emojiGrid = html_soup.find_all('ul', class_='emoji-grid')
    urls = re.findall(r'href="/+[\w\.-]+', str(emojiGrid))
    urls = [i[6:] for i in urls]

    try:
        os.mkdir('raw')
    except FileExistsError:
        pass

    print("There are {} CPUs on this machine ".format(cpu_count()))
    pool = Pool(cpu_count())
    download_func = partial(download, urls=urls)
    pool.map(download_func, urls)
    pool.close()
    pool.join()
    print("Finished Downloading!")
