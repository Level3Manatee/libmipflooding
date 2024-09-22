document.addEventListener('DOMContentLoaded', e => {
   document.querySelectorAll('.wy-menu .pre').forEach(el => {
       let fragments = el.textContent.split('_');
       let container = document.createElement('div');
       container.classList.add('break-words');
       for (let i = 0, len = fragments.length; i < len; i++) {
           fragment_el = document.createElement('span');
           fragment_el.textContent = (i > 0 ? '_' : '') + fragments[i];
           container.appendChild( fragment_el);
       }
       parent = el.closest('code');
       parent.parentNode.replaceChild(container, parent);
   });
});