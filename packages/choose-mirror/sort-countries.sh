[ -d debian/locales ] || mkdir debian/locales
[ -d debian/sort-tmp ] || mkdir debian/sort-tmp
cd debian/sort-tmp
for protocol in http ftp
do
    sed -n -e "/^Template: mirror\\/$protocol\\/countries/,/^Description:/p" ../choose-mirror/DEBIAN/templates |
    perl -p -e '
    chomp;
    if (m/Choices-([^.]*)\.UTF-8:/) {
      open (OUT, "> list.$1");
      @t = split(/, /);
      shift(@t);
      my $cnt = 1;
      foreach my $name (@t) {$cnt++; print OUT "$cnt $name\n";}
      close OUT;
    }
    $_="";'
    for file in list.*
    do
        lang=${file#list.}
        unilang=$(grep "^$lang.*\\.UTF-8" /usr/share/i18n/SUPPORTED | sed -e 1q | sed -e 's/[@. ].*//' )
        if [ -z "$unilang" ]; then
            echo "Warning: lang $lang skipped because no UTF-8 variant found" 1>&2
        else
            [ -d ../locales/$unilang.UTF-8 ] || localedef -c -f UTF-8 -i $unilang ../locales/$unilang.UTF-8
            LOCPATH=`pwd` LC_ALL=../locales/$unilang.UTF-8 sort -k 2.1 $file | sed -e 's/ .*/, /' | tr -d '\n' | sed -e "s/^/Indices-$lang.UTF-8: 1, /" -e 's/, $//' > sorted$file
        fi
    done
    #  Now ../choose-mirror/DEBIAN/templates must be patched: all sortedlist.*
    #  files have to be added to the mirror/$protocol/countries template.
done

