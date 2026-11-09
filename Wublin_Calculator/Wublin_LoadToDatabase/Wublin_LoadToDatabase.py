import json
import psycopg
import os
from dotenv import load_dotenv # learned of from https://pypi.org/project/python-dotenv/ 
def main():
    load_dotenv(dotenv_path="C:/Users/Daniel Worktop/Desktop/Wublin_LoadToDatabase/db.env")
    password = os.getenv("PASS")
    with open("C:/Users/Daniel Worktop/Desktop/Wublin_project/wublinSolutions.json", "r", encoding="utf-8") as file:
        data = json.load(file)
    with psycopg.connect(host="localhost",
                          port=5432,
                          dbname="postgres",
                          user="postgres",
                          password=password) as conn:
        count = 1
        with conn.cursor() as cur:
            for wublinLayout in data:
                #try:
                    #print (wublinLayout)
                cur.execute("INSERT INTO wublin_layout (hash_id, wublin_coords) VALUES (%s, %s) ON CONFLICT (hash_id) DO NOTHING", (wublinLayout["ID"], wublinLayout["Solution"]))
                #except: 
                    #print("WARNING: Layout number ", count, "was a database duplicate")
                count+=1
if __name__ == "__main__":
    main()




#CREATE TABLE wublin_layout (
#	hash_id			text PRIMARY KEY,
#	wublin_coords	integer[][] CHECK (array_length(wublin_coords, 2) = 2) -- Wublin count is left nonfixed to have wiggle room for the future if they add to wublin island
#);