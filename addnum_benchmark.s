# Benchmark including lw instruction

# MODIFY THIS as needed to insert nop's after instruction
# that may have data dependencies !!!

	.data
place_holder: .word	0x5555		# Test offset
nums:	.word	1,2,3,4,5,6

	.text
main:	ori	$t0, $zero, 6		# number of entries in list
	ori	$t2, $zero, nums
	andi	$t1, $zero, 0
loop:	lw	$t3, 0($t2)		# get a value from list
        nop
        nop
	add	$t1, $t3, $t1
	addi	$t0, $t0, -1		# decrement counter
	addi	$t2, $t2, 4		# advance pointer
	bne	$t0, $zero, loop	# back to add next entry
	addi	$v0, $zero, 10
	syscall				# halt

	.end	main			# Entry point of program


