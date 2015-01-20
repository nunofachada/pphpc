awk -F, 'NR>19 { gsub("\"", " "); print $2, $6, $10 * 4 }' pphpcpopulations.csv > stats.txt
