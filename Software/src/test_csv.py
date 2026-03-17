import os
from datetime import datetime
from datetime import date
import time
import csv

count = 1

parent_dir = "C:\\Users\\fizzer\\Documents\\lap_data"

current_date = date.today()

new_path = os.path.join(parent_dir, f"{current_date}")

try:
    os.mkdir(new_path)
    print(f"Created directory: {current_date}!")
except Exception as e:
    print(e)

current_time = datetime.now().strftime("%H%M%S")

status = True

while status:
    with open(os.path.join(new_path, f"{current_time}.csv"), 'a', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow([1,2,3])
        print("Row written")
    time.sleep(5)