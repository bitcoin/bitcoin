define test_used_size
	set $size_ext = 0
	usedsize $size_ext $arg0
	p $size_ext
end
