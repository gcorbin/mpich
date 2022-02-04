import re


def is_comment(entry):
    return isinstance(entry, str) and re.match(r'^\s*\$\^', entry)


def strip_comments(obj, is_comment=is_comment):
    if isinstance(obj, dict):
        filtered_dict = {}
        for key, value in obj.items():
            if is_comment(key):
                 continue
            filtered_value = strip_comments(value, is_comment)
            filtered_dict[key] = filtered_value
        return filtered_dict
    elif isinstance(obj, list):
        filtered_list = []
        for item in obj:
            filtered_item = strip_comments(item, is_comment)
            if filtered_item is not None:
                filtered_list.append(filtered_item)
        return filtered_list
    else:
        if is_comment(obj):
            return None
        else:
            return obj


if __name__ == '__main__':

    test_objects = [
        None,
        '',
        '$^comment',
        7,
        ('$^ not a comment', ),
        ['$^comment'],
        [1, 'not a comment', '!$^ not a comment', '$^ comment', ' $^ comment'],
        {'$^comment': 99},
        {'$^comment key': 'does not matter', 'key': '$^comment', 'key2': 'not a comment'},
        {
            1: ['no comment', '$^comment'],
            ('a', 'b'): 2,
            'deep': {
                 'deeper': ['way below', '$^sneaky comment'],
                 'deepest': {
                     'rock bottom': [1, '2', '$^3'],
                     'foundation':'$^1',
                     '$^ninja comment':999
                 }
            }
        }
    ]

    expected = [
        None,
        '',
        None,
        7,
        ('$^ not a comment', ),
        [],
        [1, 'not a comment', '!$^ not a comment'],
        {},
        {'key': None, 'key2': 'not a comment'},
        {
            1: ['no comment'],
            ('a', 'b'): 2,
            'deep': {
                'deeper': ['way below'],
                'deepest': {
                    'rock bottom': [1, '2'],
                    'foundation': None,
                }
            }
        }
    ]

    filtered_objects = [strip_comments(item, is_comment) for item in test_objects]

    for i in zip(test_objects, filtered_objects, expected):
        if i[1] != i[2]:
            print("Original = {}\nExpected filtered = {}\nGot {}\n".format(i[0], i[2], i[1]))
