# This script scans for Sample X numConnections Y.csv files wihin the current directory, and parses them, adding # Diff and BytesSum Diff columns to the end

import csv
import os
import re
import sys
import time
import datetime
import calendar
#2008,22-Aug,Leadership,"Satoshi Nakamoto emails Wei Dai. In the email, Nakamoto links to a ""pre-release draft"" of the white paper and asks when Dai's b-money paper was published, claiming that he wants to know this so he can cite the paper correctly in his own.",

def getMonthNum(month):
	month = month.lower().strip()
	if month.endswith(' (early)') or month.endswith(' (late)'):
		month = month.split(' ')[0]
	months_1 = ['january', 'february', 'march', 'april', 'may', 'june', 'july', 'august', 'september', 'october', 'november', 'december']
	months_2 = ['jan', 'feb', 'mar', 'apr', 'may', 'jun', 'jul', 'aug', 'sep', 'oct', 'nov', 'dec']
	if month in months_1:
		return months_1.index(month) + 1
	elif month in months_2:
		return months_2.index(month) + 1
	else:
		return -1

def daterange(start_date, end_date):
	for n in range(int ((end_date - start_date).days)):
		yield start_date + datetime.timedelta(n)

if __name__ == '__main__':
		dictionary = {}

		readerFile = open('Events.csv', 'r')
		writerFile = open('Processed_Events.csv', 'w', newline='')

		# Remove NUL bytes to prevent errors
		reader = csv.reader(x.replace('\0', '').replace('–', '-') for x in readerFile)
		writer = csv.writer(writerFile)

		for row in reader:
			if len(row) != 5: continue
			year = row[0].strip()
			month_day = row[1].strip()
			topic = row[2].strip()
			description = row[3].strip()
			country = row[4].strip()

			if not re.match(r'^[0-9]+$', year): continue
			year = int(year)
			#
			if month_day == '': # Entire year
				overwritable = True
				start_date = datetime.date(year, 1, 1)
				end_date = datetime.date(year, 12, 31)
				for single_date in daterange(start_date, end_date):
					date = single_date.strftime('%Y-%m-%d')
					if date not in dictionary:
						dictionary[date] = (overwritable, date, topic, description, country)
			# January (early)
			elif getMonthNum(month_day) != -1: # Entire month
				month = getMonthNum(month_day)

				overwritable = True
				weekday, num_days_in_month = calendar.monthrange(year, month)
				start_date = datetime.date(year, month, 1)
				end_date = datetime.date(year, month, num_days_in_month)
				
				if month_day.endswith(' (late)'):
					start_date = datetime.date(year, month, 20)
				if month_day.endswith(' (early)'):
					end_date = datetime.date(year, month, 10)

				for single_date in daterange(start_date, end_date):
					date = single_date.strftime('%Y-%m-%d')
					# If does not exist OR it's overwritable
					if date not in dictionary or dictionary[date][0] == True:
						dictionary[date] = (overwritable, date, topic, description, country)
			# 5-Jan
			elif re.match(r'[0-9]+\s*-\s*[A-Za-z]+', month_day):
				day, month_str = month_day.split('-')
				day = day.strip()
				month = getMonthNum(month_str)
				if month == -1: continue
				date = f'{year}-{month}-{day}'
				overwritable = False
				dictionary[date] = (overwritable, date, topic, description, country)
			# January - February
			elif re.match(r'[A-Za-z]+\s*-\s*[A-Za-z]+', month_day):
				month1_str, month2_str = month_day.split('-')
				month1 = getMonthNum(month1_str)
				month2 = getMonthNum(month2_str)
				if month1 == -1 or month2 == -1: continue
				overwritable = True
				num_days_in_month2 = calendar.monthrange(year, month2)[1]
				start_date = datetime.date(year, month1, 1)

				if month1_str.endswith(' (late)'):
					start_date = datetime.date(year, month1, 20)
				if month1_str.endswith(' (early)'):
					start_date = datetime.date(year, month1, 10)

				end_date = datetime.date(year, month2, num_days_in_month2)
				
				if month2_str.endswith(' (late)'):
					end_date = datetime.date(year, month2, 20)
				if month2_str.endswith(' (early)'):
					end_date = datetime.date(year, month2, 10)

				for single_date in daterange(start_date, end_date):
					date = single_date.strftime('%Y-%m-%d')
					if date not in dictionary or dictionary[date][0] == True:
						dictionary[date] = (overwritable, date, topic, description, country)
			# January 1-5
			elif re.match(r'([A-Za-z]+)\s([0-9]+)-([0-9]+)', month_day):
				match = re.match(r'([A-Za-z]+)\s([0-9]+)-([0-9]+)', month_day)
				month = getMonthNum(match.group(1))
				start_day = int(match.group(2))
				end_day = int(match.group(3))

				if month == -1: continue
				num_days_in_month = calendar.monthrange(year, month)[1]
				if start_day < 1 or start_day > num_days_in_month: continue
				if end_day < 1 or end_day > num_days_in_month: continue

				overwritable = False
				start_date = datetime.date(year, month, start_day)
				end_date = datetime.date(year, month, end_day)
				for single_date in daterange(start_date, end_date):
					date = single_date.strftime('%Y-%m-%d')
					if date not in dictionary or dictionary[date][0] == True:
						dictionary[date] = (overwritable, date, topic, description, country)
			
			else:
				print(f'Unknown month_day: "{month_day}"')
				continue

		sorted_events = sorted(dictionary.values(), key = lambda x: datetime.datetime.strptime(x[1], '%Y-%m-%d'))
		
		for event in sorted_events:
			overwritable, date, topic, description, country = event
			#description = '"' + description.replace('"', '""') + '"'
			if topic.strip() == '': topic = '-'
			row = [date, topic, description]
			writer.writerow(row)

		readerFile.close()
		writerFile.close()
		print('\nSuccessfully generated the file!')