################################################################################
# Makefile  																   #
#                                 											   #
# Description: This file contains the make rules for proxy                     #
#                                                                              #
# Authors: Ming Fang <mingf@cs.cmu.edu>                                        #
#          Yao Zhou <yaozhou@cs.cmu.edu>                                       #
# 																			   #
################################################################################

all:
	cd src;make;mv proxy ../

clean:
	rm proxy;cd src;make clean

clobber: clean
	cd src; make clobber

handin:
	make clean; cd ..; tar cvf handin.tar 15-441-project-3 --exclude .git
