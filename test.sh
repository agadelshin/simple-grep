echo "1 test"
echo 'foo blah is a bar' | ./a.out 'foo %{0} is a %{1}' -v
echo 'foo blah is a very big boat' | ./a.out 'foo %{0} is a %{1}' -v
echo 'foo blah is bar' | ./a.out 'foo %{0} is a %{1}' -v
echo 'foo blah' | ./a.out 'foo %{0} is a %{1}' -v
echo 'foo blah is' | ./a.out 'foo %{0} is a %{1}' -v
echo "2 test"
echo 'foo blah is a bar' | ./a.out 'foo %{0} is a %{1S0}' -v
echo 'foo blah is a very big boat' | ./a.out 'foo %{0} is a %{1S0}' -v
echo 'foo blah is bar' | ./a.out 'foo %{0} is a %{1S0}' -v
echo 'foo blah' | ./a.out 'foo %{0} is a %{1S0}' -v
echo 'foo blah is' | ./a.out 'foo %{0} is a %{1S0}' -v
echo "3 test"
echo "the big brown fox ran away" | ./a.out 'the %{0S1} %{1} ran away' -v
echo "4 test"
echo "bar foo bar foo bar foo bar foo" | ./a.out "bar %{0G} foo %{1}" -v
echo "5 test"
echo 'r.g{}[]|$^\()e*+?x abc' | ./a.out 'r.g{}[]|$^\()e*+?x %{0}' -v
echo 'foo' | ./a.out '%{1} jumps over %{0{' -v
echo "foo blah is a very  boat" | ./a.out "foo %{0} is a %{1S1} boat" -v
echo "big brown fox jumps over big brown fox" | ./a.out "%{1} jumps over %{1}" -v
echo "big brown fox jumps over big brown foxa" | ./a.out "%{1} jumps over %{1}" -v

