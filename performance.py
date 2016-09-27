#!usr/bin/python
import re
import os
from sys import argv

#global variables
tp = 0
fp = 0
tn = 0
fn = 0
folder = ""
output_data = open("nboutput.txt", "r", encoding="latin1")
results = {}


def getContent(path):
	global fp, tp, fn, tn, folder
	for fil in os.listdir(path):
		pathname = os.path.join(path, fil)
		if os.path.isdir(pathname):
			if fil == "ham":
				folder = "ham"
			elif fil == "spam":
				folder = "spam"
			getContent(pathname)
		elif fil.endswith('.txt'):
			label = results[pathname]
			if folder == "spam":
			    if label == "spam":
			    	tp += 1
			    else:
			    	fn += 1
			else:
				if label == "spam":
					fp += 1
				else:
					tn += 1	


def calculateStats():
	global tp, fp, fn, tn
	spam_precision = tp / (tp + fp)
	spam_recall = tp / (tp + fn)
	spam_F1_score = (2 * spam_precision * spam_recall) / (spam_precision + spam_recall)

	print("spam precision: " + str(round(spam_precision, 2)))
	print("spam recall:" + str(round(spam_recall, 2)))
	print("spam F1_score: " + str(round(spam_F1_score, 2)))

	ham_precision = tn / (tn + fn)
	ham_recall = tn / (tn + fp)
	ham_F1_score = (2 * ham_precision * ham_recall) / (ham_precision + ham_recall)

	print("ham precision: " + str(round(ham_precision, 2)))
	print("ham recall:" + str(round(ham_recall, 2)))
	print("ham F1_score: " + str(round(ham_F1_score, 2)))
    


def readOutput():
	global output_data
	output_data = output_data.read().split("\n")
	output_data = list(filter(None,output_data))
	for line in output_data:
		label = line.split()[0]
		index = line.index(" ")
		results[line[index + 1: ]] = label
	
def main():
    path = argv[1]
	readOutput()
	getContent(path)
	calculateStats()

if __name__=="__main__":
	main()