from fastapi import FastAPI, Response
from pydantic import BaseModel
import classifier

class Script(BaseModel):
    content: str

app = FastAPI()
classifier.load_context()

@app.post("/predict")
async def root(script: Script):
    prediction = classifier.get_prediction(script.content)[0]
    print("Content: ", script.content)
    print("Prediction: ", prediction)
    return Response(str(prediction))
