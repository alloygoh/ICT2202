import collections
import os
import pickle
import re

import numpy as np
import pandas as pd
from gensim.models import FastText

CLASSIFIER = None
ENCODER = None
FASTTEXT_MODEL = None

def load_context():
    global CLASSIFIER, ENCODER, FASTTEXT_MODEL
    # Load context
    FASTTEXT_MODEL = FastText.load("model.bin")
    # OneHotEncoder
    with open("enc.bin", "rb") as f:
        ENCODER = pickle.load(f)
    # Prediction
    with open("rf.bin", "rb") as f:
        CLASSIFIER = pickle.load(f)
    print("Completed loading context")

# Feature extraction
def has_url_or_ip(content: str) -> bool:
    ip_addr_regex = r"\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}"
    url_regex = r"(http.*?)['\"]?"
    has_ip = bool(re.search(ip_addr_regex, content))
    has_url = bool(re.search(url_regex, content))
    return int(has_ip or has_url)


def get_character_length_info(content: str):
    """
    Returns number of strings, max length of strings, and average length
    """
    # Get all strings
    import re

    regex = r"(['\"])(.*?)\1"
    matches = re.findall(regex, content)
    if not matches:
        return 0, 0, 0

    count, sum_length, max_length = 0, 0, 0
    for match in matches:
        string = match[1]
        string_length = len(string)

        count += 1
        sum_length += string_length

        if string_length > max_length:
            max_length = string_length

    return (count, max_length, sum_length / count)


def get_top_character_occurences(content: str, top: int = 5):
    content = content.replace(" ", "")
    return [x[0] for x in collections.Counter(content).most_common(top)]


def get_manual_features(row):
    content = row["content"].lower()
    content = re.sub(r"[\s]+", " ", content)
    row["str_count"], row["str_max"], row["str_avg"] = get_character_length_info(
        content
    )
    row["has_url_or_ip"] = has_url_or_ip(content)
    top_occurences = get_top_character_occurences(content)
    while len(top_occurences) < 5:
        top_occurences.append("\x00")
    row = pd.concat(
        [
            row,
            pd.Series(
                top_occurences,
                index=[f"top{i}" for i in range(5)],
            ),
        ]
    )
    return row


def build_fasttext_vocab(row):
    content = row["content"].lower()
    content = re.sub("[1-9]", "*", content)
    content = re.sub("[^a-zA-Z* $]", " ", content)
    content = re.sub(r"[\s]+", " ", content)
    content = content.strip()

    row["content"] = content
    return row


def build_doc_embedding(row, model):
    content = row["content"]
    words = content.split()
    num_features = 100
    feature_vec = np.zeros(
        (num_features,), dtype="float32"
    )  # pre-initialize (for speed)
    for word in words:
        feature_vec = np.add(feature_vec, model.wv[word])
    feature_vec = np.divide(feature_vec, len(words))
    return pd.concat([row, pd.Series(feature_vec, index=list(map(str, range(num_features))))])

def get_prediction(content: str):
    data = pd.DataFrame([content], columns=["content"])
    data = data.apply(get_manual_features, axis=1)
    data = data.apply(build_fasttext_vocab, axis=1)
    data = data.apply(lambda x: build_doc_embedding(x, FASTTEXT_MODEL), axis=1)

    data = data.drop(columns="content")

    transformed = ENCODER.transform(data[[f"top{i}" for i in range(5)]])
    data = (
        data.join(
            pd.DataFrame(transformed.toarray(), columns=ENCODER.get_feature_names_out())
        )
        .drop(columns=[f"top{i}" for i in range(5)])
        .fillna(0)
    )

    return CLASSIFIER.predict(data)
