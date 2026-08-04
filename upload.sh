read -p "Enter commit comment:" comment 
git add .
git commit -m "$comment"
git push origin main