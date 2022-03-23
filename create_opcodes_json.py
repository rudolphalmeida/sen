import json
from itertools import zip_longest
from typing import Callable

import requests
from bs4 import BeautifulSoup
from more_itertools import chunked

URL = "https://www.masswerk.at/6502/6502_instruction_set.html"


def find_details_table(tag) -> bool:
    return tag.has_attr("aria-label") and tag["aria-label"] == "details"


def find_aria_labeled_element(label) -> Callable:
    def func(tag) -> bool:
        return tag.has_attr("aria-label") and tag["aria-label"] == label

    return func


def extract_values_in_tr_td(tag) -> list[str]:
    return list(tag.strings)


def main():
    request = requests.get(URL)
    soup = BeautifulSoup(request.content, "html.parser")

    opcodes_json = {}

    opcodes_dl = soup.find("dl", class_="opcodes")
    for dt, dd in chunked(filter(lambda x: x != '\n', opcodes_dl.children), 2):
        # Create opcode with data from `details` table
        details = dd.find_all(find_aria_labeled_element("details"))[0]
        headings, *opcodes = filter(lambda x: x != '\n', details.children)
        headings = extract_values_in_tr_td(headings)
        opcodes = [extract_values_in_tr_td(values) for values in opcodes]

        details_dicts = [dict(zip(headings, opcode)) for opcode in opcodes]

        # Add flags field
        flags = dd.find_all(find_aria_labeled_element("flags"))[0]
        # We include the x.name != "table" check because some table tags are not closed :(
        headings, changes = list(filter(lambda x: x != '\n' and x.name != "table", flags.children))
        headings = extract_values_in_tr_td(headings)
        changes = extract_values_in_tr_td(changes)

        flags = dict(zip_longest(headings, changes))

        # Add summary field
        summary = dd.find_all(find_aria_labeled_element("summary"))[0]

        # Add synopsis field
        synopsis = dd.find_all(find_aria_labeled_element("synopsis"))[0]

        for opcode in details_dicts:
            # Remove unnecessary spaces after cycles value
            opcode["cycles"] = opcode["cycles"].strip()

            opcodes_json[opcode['opc']] = opcode
            opcodes_json[opcode['opc']]["flags"] = flags
            opcodes_json[opcode['opc']]["summary"] = summary.text
            opcodes_json[opcode['opc']]["synopsis"] = ' '.join(synopsis.strings)

    with open('opcodes.json', 'w') as opcodes_file:
        opcodes_file.write(json.dumps(opcodes_json, sort_keys=True, indent=4, separators=(',', ':')))


if __name__ == "__main__":
    main()
