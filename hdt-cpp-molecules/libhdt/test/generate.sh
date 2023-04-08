rm test.nt
rm test.hdt*

for i in {1..10000}
do
	echo "<http://example.org/subject$i> <http://example.org/predicate1> <http://example.org/object$i> ." >> test.nt
	echo "<http://example.org/subject$i> <http://example.org/predicate2> <http://example.org/object$i> ." >> test.nt
	echo "<http://example.org/subject$i> <http://example.org/predicate3> <http://example.org/object$i> ." >> test.nt
	echo "<http://example.org/subject$i> <http://example.org/predicate4> <http://example.org/object$i> ." >> test.nt
done

for i in {10001..20000}
do
	echo "<http://example.org/subject$i> <http://example.org/predicate5> <http://example.org/object$i> ." >> test.nt
done
for i in {20011..20020}
do
	echo "<http://example.org/subject$i> <http://example.org/predicate1> <http://example.org/object$i> ." >> test.nt
	echo "<http://example.org/subject$i> <http://example.org/predicate2> <http://example.org/object$i> ." >> test.nt
	echo "<http://example.org/subject$i> <http://example.org/predicate3> <http://example.org/object$i> ." >> test.nt
	echo "<http://example.org/subject$i> <http://example.org/predicate5> <http://example.org/object$i> ." >> test.nt
done
for i in {20021..20021}
do
	echo "<http://example.org/subject$i> <http://example.org/predicate3> <http://example.org/object$i> ." >> test.nt
done
for i in {20022..20022}
do
	echo "<http://example.org/subject$i> <http://example.org/predicate1> <http://example.org/object$i> ." >> test.nt
	echo "<http://example.org/subject$i> <http://example.org/predicate2> <http://example.org/object$i> ." >> test.nt
	echo "<http://example.org/subject$i> <http://example.org/predicate3> <http://example.org/object$i> ." >> test.nt
	echo "<http://example.org/subject$i> <http://example.org/predicate7> <http://example.org/object$i> ." >> test.nt
done
for i in {20023..20100}
do
	echo "<http://example.org/subject$i> <http://example.org/predicate7> <http://example.org/object$i> ." >> test.nt
done

../tools/rdf2hdt test.nt test.hdt
